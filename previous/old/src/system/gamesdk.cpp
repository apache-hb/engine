#include "gamesdk.h"

#include "system.h"
#include "gamesdk/discord_game_sdk.h"
#include "util/strings.h"
#include "logging/log.h"
#include "logging/debug.h"

#include <thread>
#include <functional>
#include "atomic-queue/queue.h"

#include "imgui/imgui.h"

using EventFn = std::function<void()>;
using CreateFn = decltype(DiscordCreate);

constexpr auto kQueueSize = 64;
constexpr auto kClientID = 434195889867849739;
constexpr auto kAppID = 434195889867849739;
constexpr auto kLibraryPath = "resources\\lib\\discord_game_sdk.dll";

constexpr std::string_view toString(EDiscordResult res) {
    switch (res) {
    case DiscordResult_Ok: return "Discord::Ok";
    case DiscordResult_ServiceUnavailable: return "Discord::ServiceUnavailable";
    case DiscordResult_InvalidVersion: return "Discord::InvalidVersion";
    case DiscordResult_LockFailed: return "Discord::LockFailed";
    case DiscordResult_InternalError: return "Discord::InternalError";
    case DiscordResult_InvalidPayload: return "Discord::InvalidPayload";
    case DiscordResult_InvalidCommand: return "Discord::InvalidCommand";
    case DiscordResult_InvalidPermissions: return "Discord::InvalidPermissions";
    case DiscordResult_NotFetched: return "Discord::NotFetched";
    case DiscordResult_NotFound: return "Discord::NotFound";
    case DiscordResult_Conflict: return "Discord::Conflict";
    case DiscordResult_InvalidSecret: return "Discord::InvalidSecret";
    case DiscordResult_InvalidJoinSecret: return "Discord::InvalidJoinSecret";
    case DiscordResult_NoEligibleActivity: return "Discord::NoEligibleActivity";
    case DiscordResult_InvalidInvite: return "Discord::InvalidInvite";
    case DiscordResult_NotAuthenticated: return "Discord::NotAuthenticated";
    case DiscordResult_InvalidAccessToken: return "Discord::InvalidAccessToken";
    case DiscordResult_ApplicationMismatch: return "Discord::ApplicationMismatch";
    case DiscordResult_InvalidDataUrl: return "Discord::InvalidDataUrl";
    case DiscordResult_InvalidBase64: return "Discord::InvalidBase64";
    case DiscordResult_NotFiltered: return "Discord::NotFiltered";
    case DiscordResult_LobbyFull: return "Discord::LobbyFull";
    case DiscordResult_InvalidLobbySecret: return "Discord::InvalidLobbySecret";
    case DiscordResult_InvalidFilename: return "Discord::InvalidFilename";
    case DiscordResult_InvalidFileSize: return "Discord::InvalidFileSize";
    case DiscordResult_InvalidEntitlement: return "Discord::InvalidEntitlement";
    case DiscordResult_NotInstalled: return "Discord::NotInstalled";
    case DiscordResult_NotRunning: return "Discord::NotRunning";
    case DiscordResult_InsufficientBuffer: return "Discord::InsufficientBuffer";
    case DiscordResult_PurchaseCanceled: return "Discord::PurchaseCanceled";
    case DiscordResult_InvalidGuild: return "Discord::InvalidGuild";
    case DiscordResult_InvalidEvent: return "Discord::InvalidEvent";
    case DiscordResult_InvalidChannel: return "Discord::InvalidChannel";
    case DiscordResult_InvalidOrigin: return "Discord::InvalidOrigin";
    case DiscordResult_RateLimited: return "Discord::RateLimited";
    case DiscordResult_OAuth2Error: return "Discord::OAuth2Error";
    case DiscordResult_SelectChannelTimeout: return "Discord::SelectChannelTimeout";
    case DiscordResult_GetGuildTimeout: return "Discord::GetGuildTimeout";
    case DiscordResult_SelectVoiceForceRequired: return "Discord::SelectVoiceForceRequired";
    case DiscordResult_CaptureShortcutAlreadyListening: return "Discord::CaptureShortcutAlreadyListening";
    case DiscordResult_UnauthorizedForAchievement: return "Discord::UnauthorizedForAchievement";
    case DiscordResult_InvalidGiftCode: return "Discord::InvalidGiftCode";
    case DiscordResult_PurchaseError: return "Discord::PurchaseError";
    case DiscordResult_TransactionAborted: return "Discord::TransactionAborted";
    default: return "Discord::Unknown";
    }
}

namespace engine::discord {
    const auto discord = system::Module(strings::widen(kLibraryPath));

    IDiscordCoreEvents kCoreEvents = nullptr;
    DiscordCreateParams kCreateParams = {
        .client_id = kClientID,
        .flags = DiscordCreateFlags_NoRequireDiscord, /// prevent the "do you want to install discord?" dialog if discord isnt installed
        .events = &kCoreEvents,
        .event_data = nullptr
    };

    struct IDiscordCore* core;
    struct IDiscordActivityManager* activityManager;

    ubn::queue<EventFn> events{kQueueSize};
    std::jthread* gamesdk = nullptr;

    void create() {
        auto discordCreate = discord.get<CreateFn>("DiscordCreate");
        if (discordCreate == nullptr) {
            log::global->fatal("failed to get DiscordCreate");
            return;
        }

        gamesdk = new std::jthread([discordCreate](auto stop) {
            auto result = discordCreate(DISCORD_VERSION, &kCreateParams, &core);
            if (result != DiscordResult_Ok) {
                log::discord->fatal("failed to create core: {}", toString(result));
                return;
            }
            activityManager = core->get_activity_manager(core);

            log::discord->info("starting thread");
            
            while (!stop.stop_requested()) {
                while (!events.is_empty()) {
                    events.pop()();
                }

                core->run_callbacks(core);
            }

            log::discord->info("flushing events");

            while (!events.is_empty()) {
                events.pop()();
            }
            core->run_callbacks(core);

            log::discord->info("stopping thread");
        });
    }

    void destroy() {
        delete gamesdk;
    }

    struct DiscordDebugObject : debug::DebugObject {
        using Super = debug::DebugObject;
        DiscordDebugObject() : Super("Discord") { }

        virtual void info() override {
            static DiscordActivity activity = {
                .type = DiscordActivityType_Playing,
                .application_id = kAppID
            };

            ImGui::Text("ClientID: %llu", kClientID);
            ImGui::Text("AppID: %llu", kAppID);
            ImGui::Text("LibraryPath: %s", kLibraryPath);
            ImGui::Separator();
            if (!discord.found()) {
                ImGui::Text("Library not found");
                return;
            }
            ImGui::Text("Activity");
            ImGui::InputText("Name", activity.name, sizeof(activity.name));
            ImGui::InputText("Details", activity.details, sizeof(activity.details));
            ImGui::InputText("State", activity.state, sizeof(activity.state));
            ImGui::Separator();
            if (ImGui::Button("Update")) {
                events.push([=] {
                    activity.timestamps.start = time(nullptr);
                    activityManager->update_activity(activityManager, &activity, nullptr, [](UNUSED void* data, auto error) {
                        log::discord->info("update-activity = {}", toString(error));
                    });
                });
            }
        }
    };

    DiscordDebugObject* kDebugObject = new DiscordDebugObject();
}
