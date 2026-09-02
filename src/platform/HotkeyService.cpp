#include "platform/HotkeyService.hpp"

#include "platform/WindowsLean.hpp"

#include <atomic>
#include <deque>
#include <future>
#include <mutex>
#include <thread>

namespace deuca::platform
{
namespace
{

constexpr wchar_t kWindowClassName[] = L"DeucaClicker.HotkeyListener";

/// Identifiant réservé à l'arrêt d'urgence. Les autres raccourcis sont
/// numérotés au-dessus, si bien qu'aucun ne peut le remplacer par accident.
constexpr int kPanicHotkeyId = 1;
constexpr int kFirstUserHotkeyId = 2;

/// Message interne qui réveille la boucle quand une demande est en attente.
constexpr UINT kProcessRequests = WM_APP + 1;

/// Délai au-delà duquel une demande adressée au fil d'écoute est abandonnée.
///
/// Le fil ne fait qu'enregistrer des raccourcis, l'opération est immédiate. Un
/// dépassement veut dire qu'il est mort ou bloqué, et attendre indéfiniment
/// figerait l'interface qui a posé la question.
constexpr auto kRequestTimeout = std::chrono::seconds{2};

[[nodiscard]] UINT toWin32Modifiers(Modifier modifiers) noexcept
{
    UINT flags = 0;

    if (contains(modifiers, Modifier::Alt))
    {
        flags |= MOD_ALT;
    }
    if (contains(modifiers, Modifier::Control))
    {
        flags |= MOD_CONTROL;
    }
    if (contains(modifiers, Modifier::Shift))
    {
        flags |= MOD_SHIFT;
    }
    if (contains(modifiers, Modifier::Windows))
    {
        flags |= MOD_WIN;
    }

    // Sans NOREPEAT, maintenir la touche enfoncée produit une rafale de
    // messages. Pour un arrêt d'urgence c'est sans danger mais inutile ; pour
    // un raccourci de bascule, ce serait un clignotement ingérable.
    return flags | MOD_NOREPEAT;
}

} // namespace

struct HotkeyService::Impl
{
    /// Une demande d'enregistrement adressée au fil d'écoute.
    ///
    /// L'action voyage avec la demande. C'est ce qui permet à la table de
    /// n'être touchée que par le fil d'écoute : si l'appelant l'y insérait
    /// lui-même, deux fils écriraient dans la même structure sans verrou.
    struct Request
    {
        bool add{true};
        int id{0};
        Hotkey hotkey{};
        HotkeyTable::Callback callback;
        std::promise<bool> result;
    };

    HotkeyTable table;
    Hotkey panicHotkey{};
    HotkeyTable::Callback panicCallback;

    std::atomic<bool> running{false};
    std::atomic<bool> panicActive{false};
    std::atomic<int> nextId{kFirstUserHotkeyId};

    std::mutex requestMutex;
    std::deque<Request> requests;

    HWND window{nullptr};
    std::atomic<DWORD> threadId{0};
    std::jthread worker;

    void run(std::stop_token token);
    void drainRequests();
    [[nodiscard]] bool submit(Request&& request);

    static LRESULT CALLBACK windowProc(HWND, UINT, WPARAM, LPARAM);
};

LRESULT CALLBACK HotkeyService::Impl::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<Impl*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr)
    {
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_HOTKEY:
        // L'action tourne sur ce fil. C'est voulu — elle doit être brève — et
        // c'est écrit dans l'en-tête pour que personne n'y mette un traitement
        // long qui bloquerait le raccourci de panique.
        self->table.dispatch(static_cast<int>(wParam));
        return 0;

    case kProcessRequests:
        self->drainRequests();
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void HotkeyService::Impl::drainRequests()
{
    for (;;)
    {
        Request request;
        {
            const std::lock_guard guard{requestMutex};
            if (requests.empty())
            {
                return;
            }
            request = std::move(requests.front());
            requests.pop_front();
        }

        bool succeeded = false;
        if (request.add)
        {
            succeeded = ::RegisterHotKey(window, request.id, toWin32Modifiers(request.hotkey.modifiers),
                                         request.hotkey.virtualKey) != FALSE;
            if (succeeded)
            {
                table.add(request.id, std::move(request.callback));
            }
        }
        else
        {
            succeeded = ::UnregisterHotKey(window, request.id) != FALSE;
            if (succeeded)
            {
                table.remove(request.id);
            }
        }

        request.result.set_value(succeeded);
    }
}

bool HotkeyService::Impl::submit(Request&& request)
{
    if (!running.load(std::memory_order_acquire))
    {
        return false;
    }

    std::future<bool> answer = request.result.get_future();

    {
        const std::lock_guard guard{requestMutex};
        requests.push_back(std::move(request));
    }

    // Le fil d'écoute est le seul à pouvoir enregistrer un raccourci pour sa
    // propre fenêtre : la demande lui est postée, et l'on attend sa réponse.
    ::PostMessageW(window, kProcessRequests, 0, 0);

    if (answer.wait_for(kRequestTimeout) != std::future_status::ready)
    {
        return false;
    }

    return answer.get();
}

void HotkeyService::Impl::run(std::stop_token token)
{
    threadId.store(::GetCurrentThreadId(), std::memory_order_release);

    const HINSTANCE instance = ::GetModuleHandleW(nullptr);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &Impl::windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;

    const ATOM registered = ::RegisterClassExW(&windowClass);
    if (registered == 0)
    {
        return;
    }

    // Fenêtre sans affichage : elle reçoit des messages sans jamais apparaître,
    // sans barre des tâches et sans pouvoir voler le focus.
    window =
        ::CreateWindowExW(0, kWindowClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
    if (window == nullptr)
    {
        ::UnregisterClassW(kWindowClassName, instance);
        return;
    }

    ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    if (panicCallback)
    {
        const bool granted = ::RegisterHotKey(window, kPanicHotkeyId, toWin32Modifiers(panicHotkey.modifiers),
                                              panicHotkey.virtualKey) != FALSE;
        if (granted)
        {
            table.add(kPanicHotkeyId, panicCallback);
        }

        panicActive.store(granted, std::memory_order_release);
    }

    running.store(true, std::memory_order_release);

    // Boucle bloquante : ce fil ne fait qu'attendre des messages, il ne doit
    // rien consommer tant qu'aucune touche n'est pressée.
    MSG message{};
    while (!token.stop_requested() && ::GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }

    running.store(false, std::memory_order_release);

    // Les raccourcis sont retirés par le fil qui les a posés, sans quoi le
    // système les garderait jusqu'à la fin du processus.
    ::UnregisterHotKey(window, kPanicHotkeyId);
    for (int id = kFirstUserHotkeyId; id < nextId.load(std::memory_order_acquire); ++id)
    {
        ::UnregisterHotKey(window, id);
    }

    ::DestroyWindow(window);
    window = nullptr;
    ::UnregisterClassW(kWindowClassName, instance);
}

HotkeyService::HotkeyService(Hotkey panicHotkey, HotkeyTable::Callback onPanic)
    : m_impl{std::make_unique<Impl>()}
{
    m_impl->panicHotkey = panicHotkey;
    m_impl->panicCallback = std::move(onPanic);

    m_impl->worker = std::jthread{[impl = m_impl.get()](std::stop_token token) { impl->run(token); }};

    // On attend que la fenêtre existe : renvoyer la main avant rendrait
    // isRunning trompeur et ferait échouer un enregistrement immédiat.
    const auto limit = std::chrono::steady_clock::now() + kRequestTimeout;
    while (!m_impl->running.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < limit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
}

HotkeyService::~HotkeyService()
{
    if (m_impl->worker.joinable())
    {
        m_impl->worker.request_stop();

        // GetMessage bloque : demander l'arrêt ne suffit pas, il faut poster un
        // message pour que la boucle reprenne la main et constate la demande.
        const DWORD id = m_impl->threadId.load(std::memory_order_acquire);
        if (id != 0)
        {
            ::PostThreadMessageW(id, WM_QUIT, 0, 0);
        }

        m_impl->worker.join();
    }
}

bool HotkeyService::isRunning() const noexcept
{
    return m_impl->running.load(std::memory_order_acquire);
}

bool HotkeyService::panicHotkeyActive() const noexcept
{
    return m_impl->panicActive.load(std::memory_order_acquire);
}

std::optional<int> HotkeyService::registerHotkey(Hotkey hotkey, HotkeyTable::Callback callback)
{
    if (!callback)
    {
        return std::nullopt;
    }

    const int id = m_impl->nextId.fetch_add(1, std::memory_order_acq_rel);

    Impl::Request request;
    request.add = true;
    request.id = id;
    request.hotkey = hotkey;
    request.callback = std::move(callback);

    if (!m_impl->submit(std::move(request)))
    {
        return std::nullopt;
    }

    return id;
}

bool HotkeyService::unregisterHotkey(int id)
{
    // L'arrêt d'urgence n'est pas retirable. C'est la seule garantie qui compte
    // dans ce fichier.
    if (id == kPanicHotkeyId || id < kFirstUserHotkeyId)
    {
        return false;
    }

    Impl::Request request;
    request.add = false;
    request.id = id;

    return m_impl->submit(std::move(request));
}

} // namespace deuca::platform
