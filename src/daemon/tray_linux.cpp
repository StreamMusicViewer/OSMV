#include "tray.hpp"
#include "common/embedded_logo.hpp"

#include <dbus/dbus.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unistd.h>

namespace osmv::daemon {

namespace {

constexpr const char* kSniObjPath = "/StatusNotifierItem";
constexpr const char* kMenuObjPath = "/MenuBar";

constexpr const char* kSniIntrospectXml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
    "    <method name=\"Get\"><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"out\"/></method>\n"
    "    <method name=\"GetAll\"><arg type=\"s\" direction=\"in\"/><arg type=\"a{sv}\" direction=\"out\"/></method>\n"
    "  </interface>\n"
    "  <interface name=\"org.kde.StatusNotifierItem\">\n"
    "    <method name=\"Activate\"><arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/></method>\n"
    "    <method name=\"SecondaryActivate\"><arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/></method>\n"
    "    <property name=\"Category\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"Id\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"Title\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"IconName\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
    "    <property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Menu\" type=\"o\" access=\"read\"/>\n"
    "  </interface>\n"
    "</node>\n";

constexpr const char* kMenuIntrospectXml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"com.canonical.dbusmenu\">\n"
    "    <method name=\"GetLayout\">\n"
    "      <arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/><arg type=\"as\" direction=\"in\"/>\n"
    "      <arg type=\"u\" direction=\"out\"/><arg type=\"(ia{sv}av)\" direction=\"out\"/>\n"
    "    </method>\n"
    "    <method name=\"AboutToShow\"><arg type=\"i\" direction=\"in\"/><arg type=\"b\" direction=\"out\"/></method>\n"
    "    <method name=\"Event\"><arg type=\"i\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"in\"/><arg type=\"u\" direction=\"in\"/></method>\n"
    "  </interface>\n"
    "</node>\n";

void append_variant_string(DBusMessageIter* iter, const char* val) {
    DBusMessageIter v;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "s", &v);
    dbus_message_iter_append_basic(&v, DBUS_TYPE_STRING, &val);
    dbus_message_iter_close_container(iter, &v);
}

void append_variant_bool(DBusMessageIter* iter, dbus_bool_t val) {
    DBusMessageIter v;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "b", &v);
    dbus_message_iter_append_basic(&v, DBUS_TYPE_BOOLEAN, &val);
    dbus_message_iter_close_container(iter, &v);
}

void append_variant_objpath(DBusMessageIter* iter, const char* path) {
    DBusMessageIter v;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "o", &v);
    dbus_message_iter_append_basic(&v, DBUS_TYPE_OBJECT_PATH, &path);
    dbus_message_iter_close_container(iter, &v);
}

void append_icon_pixmap_variant(DBusMessageIter* iter) {
    DBusMessageIter v, arr, st, bytes;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "a(iiay)", &v);
    dbus_message_iter_open_container(&v, DBUS_TYPE_ARRAY, "(iiay)", &arr);
    dbus_message_iter_open_container(&arr, DBUS_TYPE_STRUCT, nullptr, &st);
    int32_t w = assets::kTrayIconSize;
    int32_t h = assets::kTrayIconSize;
    dbus_message_iter_append_basic(&st, DBUS_TYPE_INT32, &w);
    dbus_message_iter_append_basic(&st, DBUS_TYPE_INT32, &h);
    dbus_message_iter_open_container(&st, DBUS_TYPE_ARRAY, "y", &bytes);
    const uint8_t* ptr = assets::kTrayIconArgb32;
    int total = w * h * 4;
    dbus_message_iter_append_fixed_array(&bytes, DBUS_TYPE_BYTE, &ptr, total);
    dbus_message_iter_close_container(&st, &bytes);
    dbus_message_iter_close_container(&arr, &st);
    dbus_message_iter_close_container(&v, &arr);
    dbus_message_iter_close_container(iter, &v);
}

void append_dict_sv_string(DBusMessageIter* dict, const char* key, const char* val) {
    DBusMessageIter entry;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    append_variant_string(&entry, val);
    dbus_message_iter_close_container(dict, &entry);
}

void append_menu_item_struct(DBusMessageIter* parent_variant_arr, int32_t id, const char* label) {
    DBusMessageIter v, st, props, children;
    dbus_message_iter_open_container(parent_variant_arr, DBUS_TYPE_VARIANT, "(ia{sv}av)", &v);
    dbus_message_iter_open_container(&v, DBUS_TYPE_STRUCT, nullptr, &st);
    dbus_message_iter_append_basic(&st, DBUS_TYPE_INT32, &id);
    dbus_message_iter_open_container(&st, DBUS_TYPE_ARRAY, "{sv}", &props);
    append_dict_sv_string(&props, "label", label);
    {
        DBusMessageIter entry;
        const char* k = "enabled";
        dbus_message_iter_open_container(&props, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &k);
        append_variant_bool(&entry, TRUE);
        dbus_message_iter_close_container(&props, &entry);
    }
    dbus_message_iter_close_container(&st, &props);
    dbus_message_iter_open_container(&st, DBUS_TYPE_ARRAY, "v", &children);
    dbus_message_iter_close_container(&st, &children);
    dbus_message_iter_close_container(&v, &st);
    dbus_message_iter_close_container(parent_variant_arr, &v);
}

class LinuxTrayController final : public TrayController {
public:
    ~LinuxTrayController() override {
        if (conn_) {
            dbus_connection_unref(conn_);
            conn_ = nullptr;
        }
    }

    bool init(const TrayCallbacks& callbacks) override {
        callbacks_ = callbacks;
        DBusError err;
        dbus_error_init(&err);
        conn_ = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err)) dbus_error_free(&err);
        if (!conn_) return true; // Non-fatal if session bus unavailable

        dbus_connection_set_exit_on_disconnect(conn_, FALSE);

        bus_name_ = "org.kde.StatusNotifierItem-" + std::to_string(static_cast<long>(getpid())) + "-1";
        dbus_bus_request_name(conn_, bus_name_.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
        if (dbus_error_is_set(&err)) dbus_error_free(&err);

        static const DBusObjectPathVTable sni_vt{nullptr, &LinuxTrayController::sni_msg_handler, nullptr, nullptr, nullptr, nullptr};
        static const DBusObjectPathVTable menu_vt{nullptr, &LinuxTrayController::menu_msg_handler, nullptr, nullptr, nullptr, nullptr};
        dbus_connection_register_object_path(conn_, kSniObjPath, &sni_vt, this);
        dbus_connection_register_object_path(conn_, kMenuObjPath, &menu_vt, this);

        // Register with StatusNotifierWatcher (best effort)
        DBusMessage* reg = dbus_message_new_method_call(
            "org.kde.StatusNotifierWatcher",
            "/StatusNotifierWatcher",
            "org.kde.StatusNotifierWatcher",
            "RegisterStatusNotifierItem");
        if (reg) {
            const char* bn = bus_name_.c_str();
            dbus_message_append_args(reg, DBUS_TYPE_STRING, &bn, DBUS_TYPE_INVALID);
            dbus_connection_send(conn_, reg, nullptr);
            dbus_message_unref(reg);
            dbus_connection_flush(conn_);
        }
        return true;
    }

    void set_language(const std::string& lang) override {
        std::lock_guard<std::mutex> lock(lang_mutex_);
        is_fr_ = (lang == "fr");
    }

    void run_event_loop() override {
        running_.store(true);
        while (running_.load()) {
            if (conn_) {
                dbus_connection_read_write_dispatch(conn_, 150);
            } else {
                usleep(150000);
            }
        }
    }

    void request_quit() override {
        running_.store(false);
    }

private:
    static DBusHandlerResult sni_msg_handler(DBusConnection* conn, DBusMessage* msg, void* data) {
        auto* self = static_cast<LinuxTrayController*>(data);
        if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect")) {
            DBusMessage* reply = dbus_message_new_method_return(msg);
            const char* xml = kSniIntrospectXml;
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (dbus_message_is_method_call(msg, "org.kde.StatusNotifierItem", "Activate")) {
            if (self->callbacks_.on_show_gui) self->callbacks_.on_show_gui();
            DBusMessage* reply = dbus_message_new_method_return(msg);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "Get")) {
            const char* iface = nullptr;
            const char* prop = nullptr;
            if (dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID) && prop) {
                DBusMessage* reply = dbus_message_new_method_return(msg);
                DBusMessageIter iter;
                dbus_message_iter_init_append(reply, &iter);
                if (std::strcmp(prop, "Category") == 0) append_variant_string(&iter, "ApplicationStatus");
                else if (std::strcmp(prop, "Id") == 0) append_variant_string(&iter, "osmv");
                else if (std::strcmp(prop, "Title") == 0) append_variant_string(&iter, "OSMV");
                else if (std::strcmp(prop, "Status") == 0) append_variant_string(&iter, "Active");
                else if (std::strcmp(prop, "IconName") == 0) append_variant_string(&iter, "");
                else if (std::strcmp(prop, "IconPixmap") == 0) append_icon_pixmap_variant(&iter);
                else if (std::strcmp(prop, "ItemIsMenu") == 0) append_variant_bool(&iter, FALSE);
                else if (std::strcmp(prop, "Menu") == 0) append_variant_objpath(&iter, kMenuObjPath);
                else append_variant_string(&iter, "");
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
        }
        if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Properties", "GetAll")) {
            DBusMessage* reply = dbus_message_new_method_return(msg);
            DBusMessageIter iter, dict, entry;
            dbus_message_iter_init_append(reply, &iter);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
            append_dict_sv_string(&dict, "Category", "ApplicationStatus");
            append_dict_sv_string(&dict, "Id", "osmv");
            append_dict_sv_string(&dict, "Title", "OSMV");
            append_dict_sv_string(&dict, "Status", "Active");
            append_dict_sv_string(&dict, "IconName", "");
            {
                const char* k = "IconPixmap";
                dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
                dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &k);
                append_icon_pixmap_variant(&entry);
                dbus_message_iter_close_container(&dict, &entry);
            }
            {
                const char* k = "ItemIsMenu";
                dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
                dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &k);
                append_variant_bool(&entry, FALSE);
                dbus_message_iter_close_container(&dict, &entry);
            }
            {
                const char* k = "Menu";
                dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
                dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &k);
                append_variant_objpath(&entry, kMenuObjPath);
                dbus_message_iter_close_container(&dict, &entry);
            }
            dbus_message_iter_close_container(&iter, &dict);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    static DBusHandlerResult menu_msg_handler(DBusConnection* conn, DBusMessage* msg, void* data) {
        auto* self = static_cast<LinuxTrayController*>(data);
        if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect")) {
            DBusMessage* reply = dbus_message_new_method_return(msg);
            const char* xml = kMenuIntrospectXml;
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "AboutToShow")) {
            DBusMessage* reply = dbus_message_new_method_return(msg);
            dbus_bool_t need_update = FALSE;
            dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &need_update, DBUS_TYPE_INVALID);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "GetLayout")) {
            bool fr = false;
            {
                std::lock_guard<std::mutex> lock(self->lang_mutex_);
                fr = self->is_fr_;
            }
            DBusMessage* reply = dbus_message_new_method_return(msg);
            DBusMessageIter iter, root_st, props, children;
            dbus_message_iter_init_append(reply, &iter);
            uint32_t rev = 1;
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &rev);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, nullptr, &root_st);
            int32_t root_id = 0;
            dbus_message_iter_append_basic(&root_st, DBUS_TYPE_INT32, &root_id);
            dbus_message_iter_open_container(&root_st, DBUS_TYPE_ARRAY, "{sv}", &props);
            append_dict_sv_string(&props, "children-display", "submenu");
            dbus_message_iter_close_container(&root_st, &props);
            dbus_message_iter_open_container(&root_st, DBUS_TYPE_ARRAY, "v", &children);
            append_menu_item_struct(&children, 1, fr ? "Afficher la configuration" : "Show Configuration");
            append_menu_item_struct(&children, 2, fr ? "Quitter OSMV" : "Quit OSMV");
            dbus_message_iter_close_container(&root_st, &children);
            dbus_message_iter_close_container(&iter, &root_st);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (dbus_message_is_method_call(msg, "com.canonical.dbusmenu", "Event")) {
            int32_t id = 0;
            const char* ev = nullptr;
            DBusMessageIter iter;
            if (dbus_message_iter_init(msg, &iter) && dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT32) {
                dbus_message_iter_get_basic(&iter, &id);
                dbus_message_iter_next(&iter);
                if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
                    dbus_message_iter_get_basic(&iter, &ev);
                }
            }
            if (ev && std::strcmp(ev, "clicked") == 0) {
                if (id == 1 && self->callbacks_.on_show_gui) self->callbacks_.on_show_gui();
                else if (id == 2 && self->callbacks_.on_quit) self->callbacks_.on_quit();
            }
            DBusMessage* reply = dbus_message_new_method_return(msg);
            dbus_connection_send(conn, reply, nullptr);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    DBusConnection* conn_ = nullptr;
    std::string bus_name_;
    TrayCallbacks callbacks_;
    std::atomic<bool> running_{false};
    std::mutex lang_mutex_;
    bool is_fr_ = true;
};

} // namespace

std::unique_ptr<TrayController> create_tray_controller() {
    return std::make_unique<LinuxTrayController>();
}

} // namespace osmv::daemon
