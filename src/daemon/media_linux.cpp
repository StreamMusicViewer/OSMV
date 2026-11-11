#include "media_provider.hpp"

#include <dbus/dbus.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>

namespace osmv::daemon {

namespace {

std::string extract_player_label(const std::string& bus_name) {
    constexpr const char* kPrefix = "org.mpris.MediaPlayer2.";
    if (bus_name.rfind(kPrefix, 0) == 0) {
        std::string tail = bus_name.substr(std::strlen(kPrefix));
        auto dot = tail.find('.');
        if (dot != std::string::npos) tail = tail.substr(0, dot);
        if (!tail.empty()) {
            tail[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(tail[0])));
            return tail;
        }
    }
    return "MPRIS";
}

bool query_player_properties(DBusConnection* conn, const std::string& bus_name, PolledMedia& out) {
    DBusMessage* msg = dbus_message_new_method_call(
        bus_name.c_str(),
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "GetAll");
    if (!msg) return false;

    const char* iface = "org.mpris.MediaPlayer2.Player";
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_INVALID);

    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 400, &err);
    dbus_message_unref(msg);
    if (!reply || dbus_error_is_set(&err)) {
        if (reply) dbus_message_unref(reply);
        dbus_error_free(&err);
        return false;
    }

    out = PolledMedia{};
    out.player_name = extract_player_label(bus_name);
    out.song.status = "closed";

    DBusMessageIter root_iter;
    if (dbus_message_iter_init(reply, &root_iter) &&
        dbus_message_iter_get_arg_type(&root_iter) == DBUS_TYPE_ARRAY) {
        DBusMessageIter dict_iter;
        dbus_message_iter_recurse(&root_iter, &dict_iter);
        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry_iter;
            dbus_message_iter_recurse(&dict_iter, &entry_iter);
            const char* key = nullptr;
            if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic(&entry_iter, &key);
                dbus_message_iter_next(&entry_iter);
                if (key && dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_VARIANT) {
                    DBusMessageIter var_iter;
                    dbus_message_iter_recurse(&entry_iter, &var_iter);

                    if (std::strcmp(key, "PlaybackStatus") == 0 &&
                        dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_STRING) {
                        const char* st = nullptr;
                        dbus_message_iter_get_basic(&var_iter, &st);
                        if (st) {
                            if (std::strcmp(st, "Playing") == 0) {
                                out.song.status = "playing";
                                out.song.is_playing = true;
                            } else if (std::strcmp(st, "Paused") == 0) {
                                out.song.status = "paused";
                            } else if (std::strcmp(st, "Stopped") == 0) {
                                out.song.status = "stopped";
                            }
                        }
                    } else if (std::strcmp(key, "Metadata") == 0 &&
                               dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_ARRAY) {
                        DBusMessageIter meta_iter;
                        dbus_message_iter_recurse(&var_iter, &meta_iter);
                        while (dbus_message_iter_get_arg_type(&meta_iter) == DBUS_TYPE_DICT_ENTRY) {
                            DBusMessageIter mentry;
                            dbus_message_iter_recurse(&meta_iter, &mentry);
                            const char* mkey = nullptr;
                            if (dbus_message_iter_get_arg_type(&mentry) == DBUS_TYPE_STRING) {
                                dbus_message_iter_get_basic(&mentry, &mkey);
                                dbus_message_iter_next(&mentry);
                                if (mkey && dbus_message_iter_get_arg_type(&mentry) == DBUS_TYPE_VARIANT) {
                                    DBusMessageIter mvar;
                                    dbus_message_iter_recurse(&mentry, &mvar);
                                    int vtype = dbus_message_iter_get_arg_type(&mvar);
                                    if (std::strcmp(mkey, "xesam:title") == 0 && vtype == DBUS_TYPE_STRING) {
                                        const char* v = nullptr;
                                        dbus_message_iter_get_basic(&mvar, &v);
                                        if (v) out.song.title = v;
                                    } else if (std::strcmp(mkey, "xesam:album") == 0 && vtype == DBUS_TYPE_STRING) {
                                        const char* v = nullptr;
                                        dbus_message_iter_get_basic(&mvar, &v);
                                        if (v) out.song.album = v;
                                    } else if (std::strcmp(mkey, "mpris:artUrl") == 0 && vtype == DBUS_TYPE_STRING) {
                                        const char* v = nullptr;
                                        dbus_message_iter_get_basic(&mvar, &v);
                                        if (v) out.song.art_url = v;
                                    } else if (std::strcmp(mkey, "xesam:artist") == 0 && vtype == DBUS_TYPE_ARRAY) {
                                        DBusMessageIter arr_iter;
                                        dbus_message_iter_recurse(&mvar, &arr_iter);
                                        if (dbus_message_iter_get_arg_type(&arr_iter) == DBUS_TYPE_STRING) {
                                            const char* v = nullptr;
                                            dbus_message_iter_get_basic(&arr_iter, &v);
                                            if (v) out.song.artist = v;
                                        }
                                    }
                                }
                            }
                            dbus_message_iter_next(&meta_iter);
                        }
                    }
                }
            }
            dbus_message_iter_next(&dict_iter);
        }
    }

    dbus_message_unref(reply);
    if (out.song.title.empty() && out.song.status != "closed") {
        out.song.title = "Unknown Title";
    }
    if (out.song.artist.empty() && out.song.status != "closed") {
        out.song.artist = "Unknown Artist";
    }
    return true;
}

class LinuxMediaProvider final : public MediaProvider {
public:
    ~LinuxMediaProvider() override {
        if (conn_) {
            dbus_connection_unref(conn_);
            conn_ = nullptr;
        }
    }

    PolledMedia current_song() override {
        PolledMedia empty;
        empty.song.status = "closed";

        if (!ensure_connection()) return empty;

        DBusMessage* msg = dbus_message_new_method_call(
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            "ListNames");
        if (!msg) return empty;

        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn_, msg, 400, &err);
        dbus_message_unref(msg);
        if (!reply || dbus_error_is_set(&err)) {
            if (reply) dbus_message_unref(reply);
            dbus_error_free(&err);
            dbus_connection_unref(conn_);
            conn_ = nullptr;
            return empty;
        }

        std::vector<std::string> players;
        DBusMessageIter iter;
        if (dbus_message_iter_init(reply, &iter) &&
            dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
            DBusMessageIter arr;
            dbus_message_iter_recurse(&iter, &arr);
            while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
                const char* name = nullptr;
                dbus_message_iter_get_basic(&arr, &name);
                if (name && std::strncmp(name, "org.mpris.MediaPlayer2.", 23) == 0) {
                    players.emplace_back(name);
                }
                dbus_message_iter_next(&arr);
            }
        }
        dbus_message_unref(reply);

        if (players.empty()) return empty;

        // Prefer playerctld if available, otherwise pick first playing or first valid player
        PolledMedia fallback;
        bool has_fallback = false;
        for (const auto& bus : players) {
            PolledMedia candidate;
            if (!query_player_properties(conn_, bus, candidate)) continue;
            if (candidate.song.is_playing) {
                return candidate;
            }
            if (!has_fallback) {
                fallback = std::move(candidate);
                has_fallback = true;
            }
        }
        return has_fallback ? fallback : empty;
    }

private:
    bool ensure_connection() {
        if (conn_ && dbus_connection_get_is_connected(conn_)) return true;
        if (conn_) {
            dbus_connection_unref(conn_);
            conn_ = nullptr;
        }
        DBusError err;
        dbus_error_init(&err);
        conn_ = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err)) {
            dbus_error_free(&err);
        }
        if (!conn_) return false;
        dbus_connection_set_exit_on_disconnect(conn_, FALSE);
        return true;
    }

    DBusConnection* conn_ = nullptr;
};

} // namespace

std::unique_ptr<MediaProvider> create_media_provider() {
    return std::make_unique<LinuxMediaProvider>();
}

} // namespace osmv::daemon
