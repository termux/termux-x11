#pragma once
#include <string>
#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/xinput.h>
#include <xcb/xfixes.h>
#include <xcb/xtest.h>
#include <xcb/shm.h>
#include <xcb/randr.h>
#include <xcb_errors.h>

#define explicit c_explicit
#include <xcb/xkb.h>
#undef explicit

#include <android/input.h>

#define xcb(name, ...) xcb_ ## name ## _reply(self.conn, xcb_ ## name(self.conn, ## __VA_ARGS__), &self.err)
#define xcb_check(name, ...) self.err = xcb_request_check(self.conn, xcb_ ## name(self.conn, ## __VA_ARGS__))

[[maybe_unused]] typedef uint8_t u8;
[[maybe_unused]] typedef uint16_t u16;
[[maybe_unused]] typedef uint32_t u32;
[[maybe_unused]] typedef uint64_t u64;
[[maybe_unused]] typedef int8_t i8;
[[maybe_unused]] typedef int16_t i16;
[[maybe_unused]] typedef int32_t i32;
[[maybe_unused]] typedef int64_t i64;

class xcb_connection {
private:

public:
    xcb_connection_t *conn{};
    xcb_generic_error_t* err{}; // not thread-safe, but the whole class should be used in separate thread
    xcb_errors_context_t *err_ctx{};

    template<typename REPLY>
    always_inline void handle_error(REPLY* reply, std::string description) { // NOLINT(performance-unnecessary-value-param)
        if (err) {
            const char* ext{};
            const char* err_name =  xcb_errors_get_name_for_error(err_ctx, err->error_code, &ext);
            std::string err_text = description + "\n" +
                                   "XCB Error of failed request:               " + (ext?:"") + "::" + err_name + "\n" +
                                   "  Major opcode of failed request:          " + std::to_string(err->major_code) + " (" +
                                   xcb_errors_get_name_for_major_code(err_ctx, err->major_code) + ")\n" +
                                   "  Minor opcode of failed request:          " + std::to_string(err->minor_code) + " (" +
                                   xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code) + ")\n" +
                                   "  Serial number of failed request:         " + std::to_string(err->sequence) + "\n" +
                                   "  Current serial number in output stream:  " + std::to_string(err->full_sequence);

            free(reply);
            free(err);
            err = nullptr;
            throw std::runtime_error(err_text);
        }
    }

    always_inline void handle_error(std::string description) { // NOLINT(performance-unnecessary-value-param)
        if (err) {
            const char* ext{};
            const char* err_name =  xcb_errors_get_name_for_error(err_ctx, err->error_code, &ext);
            std::string err_text = description + "\n" +
                                   "XCB Error of failed request:               " + (ext?:"") + "::" + err_name + "\n" +
                                   "  Major opcode of failed request:          " + std::to_string(err->major_code) + " (" +
                                   xcb_errors_get_name_for_major_code(err_ctx, err->major_code) + ")\n" +
                                   "  Minor opcode of failed request:          " + std::to_string(err->minor_code) + " (" +
                                   xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code) + ")\n" +
                                   "  Serial number of failed request:         " + std::to_string(err->sequence) + "\n" +
                                   "  Current serial number in output stream:  " + std::to_string(err->full_sequence);
            free(err);
            err = nullptr;
            throw std::runtime_error(err_text);
        }
    }

    struct {
        xcb_connection& self;
        i32 first_event{};
        void init() {
            {
                auto reply = xcb(shm_query_version);
                self.handle_error(reply, "Error querying MIT-SHM extension");
                free(reply);
            }
            {
                auto reply = xcb(query_extension, 6, "DAMAGE");
                self.handle_error(reply, "Error querying DAMAGE extension");
                first_event = reply->first_event;
                free(reply);
            }
        };

        void attach_fd(u32 seg, i32 fd, u8 ro) {
            xcb_check(shm_attach_fd, seg, fd, ro);
            self.handle_error("Error attaching file descriptor through MIT-SHM extension");
        };

        int create_segment(u32 seg, u32 size, u8 ro) {
            int fd;
            auto reply = xcb(shm_create_segment, seg, size, ro);
            self.handle_error("Error creating shared segment through MIT-SHM extension");
            if (reply->nfd != 1) {
                std::runtime_error("Error creating shared segment through MIT-SHM extension: did not get file descriptor");
            }
            fd = xcb_shm_create_segment_reply_fds(self.conn, reply)[0];
            free(reply);
            return fd;
        }

        [[maybe_unused]] void detach(u32 seg) {
            xcb_check(shm_detach, seg);
            self.handle_error("Error attaching shared segment through MIT-SHM extension");
        }

        xcb_shm_get_image_reply_t* get(xcb_drawable_t d, i16 x, i16 y, i16 w, i16 h, u32 m, u8 f, xcb_shm_seg_t s, u32 o) {
            auto reply = xcb(shm_get_image, d, x, y, w, h, m, f, s, o);
            self.handle_error(reply, "Error getting shm image through MIT-SHM extension");
            return reply;
        };
    } shm {*this};

    struct {
        xcb_connection& self;
        i32 first_event{};

        void init() {
            {
                auto reply = xcb(query_extension, 6, "DAMAGE");
                self.handle_error(reply, "Error querying DAMAGE extension");
                first_event = reply->first_event;
                free(reply);
            }
            {
                auto reply = xcb(damage_query_version, 1, 1);
                self.handle_error(reply, "Error querying DAMAGE extension");
                free(reply);
            }
        }

        void create(xcb_drawable_t d, uint8_t l) {
            xcb_check(damage_create, xcb_generate_id(self.conn), d, l);
            self.handle_error("Error creating damage");
        }

        inline bool is_damage_notify_event(xcb_generic_event_t *ev) const {
            return ev->response_type == (first_event + XCB_DAMAGE_NOTIFY);
        }
    } damage {*this};

    struct {
        xcb_connection& self;
        i32 opcode{};
        void init() {
            {
                auto reply = xcb(query_extension, 15, "XInputExtension");
                self.handle_error(reply, "Error querying XInputExtension extension");
                opcode = reply->major_opcode;
                free(reply);
            }
            {
                auto reply = xcb(input_get_extension_version, 15, "XInputExtension");
                self.handle_error(reply, "Error querying XInputExtension extension");
                free(reply);
            }
            {
                auto reply = xcb(input_xi_query_version, 2, 2);
                self.handle_error(reply, "Error querying XInputExtension extension");
                free(reply);
            }
        }

        xcb_input_device_id_t client_pointer_id() {
            xcb_input_device_id_t id;
            auto reply = xcb(input_xi_get_client_pointer, XCB_NONE);
            self.handle_error(reply, "Error getting client pointer device id");
            id = reply->deviceid;
            free(reply);
            return id;
        }

        void select_events(xcb_window_t window, uint16_t num_mask, const xcb_input_event_mask_t *masks){
            xcb_check(input_xi_select_events, window, num_mask, masks);
            self.handle_error("Error selecting Xi events");
        }

        [[maybe_unused]] inline bool is_raw_motion_event(xcb_generic_event_t *ev) const {
            union { // NOLINT(cppcoreguidelines-pro-type-member-init)
                xcb_generic_event_t *event;
                xcb_ge_event_t *ge;
            };
            event = ev;
            return ev->response_type == XCB_GE_GENERIC && /* cookie */ ge->pad0 == opcode && ge->event_type == XCB_INPUT_RAW_MOTION;
        }
    } input {*this};

    struct {
        xcb_connection& self;
        int first_event{};
        void init() {
            {
                auto reply = xcb(query_extension, 6, "XFIXES");
                self.handle_error(reply, "Error querying XFIXES extension");
                first_event = reply->first_event;
                free(reply);
            }
            {
                auto reply = xcb(xfixes_query_version, 4, 0);
                self.handle_error(reply, "Error querying XFIXES extension");
                free(reply);
            }
        }

        void select_input(xcb_window_t window, uint32_t mask) {
            xcb_check(xfixes_select_cursor_input, window, mask);
            self.handle_error("Error querying selecting XFIXES input");
        }

        bool is_cursor_notify_event(xcb_generic_event_t* e) const {
            return e->response_type == first_event + XCB_XFIXES_CURSOR_NOTIFY;
        }

         xcb_xfixes_get_cursor_image_reply_t* get_cursor_image() {
            auto reply = xcb(xfixes_get_cursor_image);
            self.handle_error(reply, "Error getting XFIXES cursor image");
            return reply;
        }
    } fixes {*this};

    struct {
        xcb_connection &self;
        void init() {
            auto reply = xcb(test_get_version, 2, 2);
            self.handle_error(reply, "Error querying XFIXES extension");
            free(reply);
        }
    } xtest {*this};

    struct {
        xcb_connection &self;
        xkb_context* context{};
        int core_kbd{};

        struct code {
            uint32_t ucs;
            xkb_keysym_t keysym;
            xkb_keycode_t keycode;
            xkb_layout_index_t layout;
            xkb_mod_mask_t modifiers;
        };

        struct binding {
            uint32_t ucs;
            xkb_keycode_t keycode;
            timespec expires;
        };

        std::vector<code> codes;
        std::vector<binding> bindings;
        void init() {
            if (context) {
                xkb_context_unref(context);
            }

            context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
            if (!context)
                throw std::runtime_error("Failed to setup xkb_context");

            if (!xkb_x11_setup_xkb_extension(self.conn,
                                             XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                             XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr,
                                             nullptr, nullptr))

                throw std::runtime_error("Failed to setup xkb extension");

            enum {
                required_events =
                (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                 XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                 XCB_XKB_EVENT_TYPE_STATE_NOTIFY),

                required_nkn_details =
                (XCB_XKB_NKN_DETAIL_KEYCODES),

                required_map_parts =
                (XCB_XKB_MAP_PART_KEY_TYPES |
                 XCB_XKB_MAP_PART_KEY_SYMS |
                 XCB_XKB_MAP_PART_MODIFIER_MAP |
                 XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                 XCB_XKB_MAP_PART_KEY_ACTIONS |
                 XCB_XKB_MAP_PART_VIRTUAL_MODS |
                 XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP),

                required_state_details =
                (XCB_XKB_STATE_PART_MODIFIER_BASE |
                 XCB_XKB_STATE_PART_MODIFIER_LATCH |
                 XCB_XKB_STATE_PART_MODIFIER_LOCK |
                 XCB_XKB_STATE_PART_GROUP_BASE |
                 XCB_XKB_STATE_PART_GROUP_LATCH |
                 XCB_XKB_STATE_PART_GROUP_LOCK),
            };

            static const xcb_xkb_select_events_details_t details = {
                    .affectNewKeyboard = required_nkn_details,
                    .newKeyboardDetails = required_nkn_details,
                    .affectState = required_state_details,
                    .stateDetails = required_state_details,
            };

            core_kbd = xkb_x11_get_core_keyboard_device_id(self.conn);
            if (core_kbd == -1)
                throw std::runtime_error("Failed to get core keyboard id");

            xcb_check(xkb_select_events, XCB_XKB_ID_USE_CORE_KBD, required_events, 0, 0, required_map_parts, required_map_parts, &details);
            self.handle_error("Failed to select xkb events");

            reload_keymaps(true);
        }

        void reload_keymaps(bool reload_bindings = false) {
            auto keymap = xkb_x11_keymap_new_from_device(context, self.conn, core_kbd, XKB_KEYMAP_COMPILE_NO_FLAGS);

            codes.clear();

            auto iter = [&](xkb_keycode_t key) {
                int count;
                const xkb_keysym_t *sym;
                for (auto &i: bindings) {
                    // in the case if some other app like x11vnc or chrome-remote-desktop rebinded this keycode...
                    if (i.keycode == key) {
                        if (xkb_keymap_num_layouts_for_key(keymap, key) == 0) {
                            if (i.ucs != 0)
                                i = {0, i.keycode, now() + ms_to_timespec(200)};
                        } else {
                            count = xkb_keymap_key_get_syms_by_level(keymap, key, 0, 0, &sym);
                            i = {(count)?(*sym):0, key, now() + ms_to_timespec(200)};
                        }
                        return;
                    }
                }
                if (xkb_keymap_num_layouts_for_key(keymap, key) == 0) {
                    // if this key has no bindings it is interesting for rebinding purposes...
                    // it will only happen if this key not in bindings yet...
                    bindings.push_back({0, key, now()});
                } else {
                    for (xkb_layout_index_t layout=0; layout < xkb_keymap_num_layouts_for_key(keymap, key); layout++)
                        for (xkb_level_index_t level=0; level < xkb_keymap_num_levels_for_key(keymap, key, layout); level++) {
                            count = xkb_keymap_key_get_syms_by_level(keymap, key, layout, level, &sym);
                            if (count && *sym != 0) {
                                uint32_t ucs = xkb_keysym_to_utf32(*sym);
                                xkb_mod_mask_t mod = 0;
                                xkb_keymap_key_get_mods_for_level(keymap, key, layout, level, &mod, 1);
                                codes.push_back({ucs, *sym, key, layout, mod});
                            }
                        }
                }
            };
            xkb_keymap_key_for_each(keymap, [](xkb_keymap *, xkb_keycode_t key, void *d) {
                (*reinterpret_cast<decltype(iter)*>(d))(key);
            }, static_cast<void *>(&iter));

            // `codes` will be used in binary search, so we should sort it by unicode symbol value
            std::sort(codes.begin(), codes.end(), [](code& a, code& b){ return a.ucs < b.ucs; });
        }

        void send_keysym(xcb_keysym_t keysym, int meta_state) {
            char buf[64]{};
            xkb_keysym_get_name(keysym, buf, sizeof(buf));
            ALOGV("Sending keysym %s", buf);

            auto code = std::find_if(codes.begin(), codes.end(), [&c = keysym](auto& code) {
                return code.keysym == c;
            });

            if (code == codes.end()) {
                xkb_keysym_get_name(keysym, buf, sizeof(buf));
                ALOGE("Code for keysym 0x%X (%s) not found...", keysym, buf);
                return;
            }

            xcb_flush(self.conn);

            xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(self.conn)).data;

            // Since we do not track state of modifiers we should reset them before we finish here.
            // Getting current modifiers
            u32 modifiers = code->modifiers;
//            {
//                auto reply = xcb(query_pointer, s->root);
//                if (reply)
//                    modifiers &= ~reply->mask;
//
//                ALOGE("pointer mods %d", reply->mask);
//                free(reply);
//            }

            u8 current_group = 0;
            {
                auto reply = xcb(xkb_get_state, XCB_XKB_ID_USE_CORE_KBD);
                current_group = reply->group;
                modifiers &= ~reply->mods;
                ALOGE("pointer mods %d", reply->mods);
                free(reply);
            }

            if (meta_state & AMETA_CTRL_ON)
                modifiers |= XCB_MOD_MASK_CONTROL;

            if (meta_state & AMETA_SHIFT_ON)
                modifiers |= XCB_MOD_MASK_SHIFT;

            if (meta_state & AMETA_ALT_ON)
                modifiers |= XCB_MOD_MASK_1;

            if (meta_state & AMETA_META_ON)
                modifiers |= XCB_MOD_MASK_4;

            auto reply = xcb(get_modifier_mapping);
            self.handle_error("Failed to get keyboard modifiers.");

            for (int mod_index = XCB_MAP_INDEX_SHIFT; mod_index <= XCB_MAP_INDEX_5; mod_index++) {
                if (modifiers & (1 << mod_index)) {
                    for (int mod_key = 0; mod_key < reply->keycodes_per_modifier; mod_key++) {
                        int keycode = xcb_get_modifier_mapping_keycodes(reply)
                            [mod_index * reply->keycodes_per_modifier + mod_key];
                        if (keycode)
                            xcb_test_fake_input(self.conn, XCB_KEY_PRESS, keycode, XCB_CURRENT_TIME,
                                                s->root, 0, 0, 0);
                    }
                }
            }

            xcb_xkb_latch_lock_state(self.conn, XCB_XKB_ID_USE_CORE_KBD, 0, 0, 1, code->layout, 0, 0, 0);
            xcb_test_fake_input(self.conn, XCB_KEY_PRESS, code->keycode, XCB_CURRENT_TIME, s->root, 0, 0, 0);
            xcb_test_fake_input(self.conn, XCB_KEY_RELEASE, code->keycode, XCB_CURRENT_TIME, s->root, 0, 0, 0);
            xcb_xkb_latch_lock_state(self.conn, XCB_XKB_ID_USE_CORE_KBD, 0, 0, 1, current_group, 0, 0, 0);

            for (int mod_index = XCB_MAP_INDEX_SHIFT; mod_index <= XCB_MAP_INDEX_5; mod_index++) {
                if (modifiers & (1 << mod_index)) {
                    for (int mod_key = 0; mod_key < reply->keycodes_per_modifier; mod_key++) {
                        int keycode = xcb_get_modifier_mapping_keycodes(reply)
                            [mod_index * reply->keycodes_per_modifier + mod_key];
                        if (keycode)
                            xcb_test_fake_input(self.conn, XCB_KEY_RELEASE, keycode,
                                                XCB_CURRENT_TIME, s->root, 0, 0, 0);
                    }
                }
            }

            free(reply);
            xcb_flush(self.conn);
        }

        void send_key(xcb_keysym_t keysym, u8 type) {
            // SurfaceView can only receive Unicode input from a virtual keyboard.
            // Since SurfaceView is not a text editor, it cannot receive Unicode input from a
            // physical keyboard. This is actually beneficial for us as we receive original
            // input events, albeit with encoded keycodes. Since we receive original keycodes
            // regardless of the layout, we can simply pass them to X11 as is, and the desktop
            // environment will determine in which layout the user will be typing.
            // Here we do not change current layout and do not change modifiers because
            // this function should only be triggered by hardware keyboard or extra key bar.
            char buf[64]{};
            xkb_keysym_get_name(keysym, buf, sizeof(buf));
            ALOGV("Sending keysym %s", buf);

            auto code = std::find_if(codes.begin(), codes.end(), [&c = keysym](auto& code) {
                return code.keysym == c;
            });

            if (code == codes.end()) {
                xkb_keysym_get_name(keysym, buf, sizeof(buf));
                ALOGE("Code for keysym 0x%X (%s) not found...", keysym, buf);
                return;
            }

            xcb_flush(self.conn);

            xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(self.conn)).data;
            xcb_test_fake_input(self.conn, type, code->keycode, XCB_CURRENT_TIME, s->root, 0, 0, 0);
            xcb_flush(self.conn);
        }
    } xkb {*this};

    void init(int sockfd) {
        xcb_connection_t* new_conn = xcb_connect_to_fd(sockfd, nullptr);
        int conn_err = xcb_connection_has_error(new_conn);
        if (conn_err) {
            const char *s;
            switch (conn_err) {
#define c(name) case name: s = static_cast<const char*>(#name); break
                c(XCB_CONN_ERROR);
                c(XCB_CONN_CLOSED_EXT_NOTSUPPORTED);
                c(XCB_CONN_CLOSED_MEM_INSUFFICIENT);
                c(XCB_CONN_CLOSED_REQ_LEN_EXCEED);
                c(XCB_CONN_CLOSED_PARSE_ERR);
                c(XCB_CONN_CLOSED_INVALID_SCREEN);
                c(XCB_CONN_CLOSED_FDPASSING_FAILED);
                default:
                    s = "UNKNOWN";
#undef c
            }
            throw std::runtime_error(std::string() + "XCB connection has error: " + s);
        }

        if (err_ctx)
            xcb_errors_context_free(err_ctx);
        if (conn)
            xcb_disconnect(conn);
        conn = new_conn;
        xcb_errors_context_new(conn, &err_ctx);

        shm.init();
        xtest.init();
        damage.init();
        input.init();
        fixes.init();
        xkb.init();
    }
};
