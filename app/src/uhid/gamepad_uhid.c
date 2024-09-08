#include "gamepad_uhid.h"

#include "hid/hid_gamepad.h"
#include "input_events.h"
#include "util/log.h"
#include "util/str.h"

/** Downcast gamepad processor to sc_gamepad_uhid */
#define DOWNCAST(GP) container_of(GP, struct sc_gamepad_uhid, gamepad_processor)

static void
sc_gamepad_uhid_send_input(struct sc_gamepad_uhid *gamepad,
                           const struct sc_hid_input *hid_input,
                           const char *name) {
    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_UHID_INPUT;
    msg.uhid_input.id = hid_input->hid_id;

    assert(hid_input->size <= SC_HID_MAX_SIZE);
    memcpy(msg.uhid_input.data, hid_input->data, hid_input->size);
    msg.uhid_input.size = hid_input->size;

    if (!sc_controller_push_msg(gamepad->controller, &msg)) {
        LOGE("Could not push UHID_INPUT message (%s)", name);
    }
}

static void
sc_gamepad_uhid_send_open(struct sc_gamepad_uhid *gamepad,
                          const struct sc_hid_open *hid_open) {
    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_UHID_CREATE;
    msg.uhid_create.id = hid_open->hid_id;
    msg.uhid_create.report_desc = hid_open->report_desc;
    msg.uhid_create.report_desc_size = hid_open->report_desc_size;

    if (!sc_controller_push_msg(gamepad->controller, &msg)) {
        LOGE("Could not push UHID_CREATE message (gamepad)");
    }
}

static void
sc_gamepad_uhid_send_close(struct sc_gamepad_uhid *gamepad,
                           const struct sc_hid_close *hid_close) {
    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_UHID_DESTROY;
    msg.uhid_create.id = hid_close->hid_id;

    if (!sc_controller_push_msg(gamepad->controller, &msg)) {
        LOGE("Could not push UHID_DESTROY message (gamepad)");
    }
}

static void
sc_gamepad_processor_process_gamepad_device(struct sc_gamepad_processor *gp,
                                const struct sc_gamepad_device_event *event) {
    struct sc_gamepad_uhid *gamepad = DOWNCAST(gp);

    if (event->type == SC_GAMEPAD_DEVICE_ADDED) {
        struct sc_hid_open hid_open;
        if (!sc_hid_gamepad_generate_open(&gamepad->hid, &hid_open,
                                          event->gamepad_id)) {
            return;
        }

        sc_gamepad_uhid_send_open(gamepad, &hid_open);
    } else {
        assert(event->type == SC_GAMEPAD_DEVICE_REMOVED);

        struct sc_hid_close hid_close;
        if (!sc_hid_gamepad_generate_close(&gamepad->hid, &hid_close,
                                           event->gamepad_id)) {
            return;
        }

        sc_gamepad_uhid_send_close(gamepad, &hid_close);
    }
}

static void
sc_gamepad_processor_process_gamepad_axis(struct sc_gamepad_processor *gp,
                                const struct sc_gamepad_axis_event *event) {
    struct sc_gamepad_uhid *gamepad = DOWNCAST(gp);

    struct sc_hid_input hid_input;
    if (!sc_hid_gamepad_generate_input_from_axis(&gamepad->hid, &hid_input,
                                                 event)) {
        return;
    }

    sc_gamepad_uhid_send_input(gamepad, &hid_input, "gamepad axis");
}

static void
sc_gamepad_processor_process_gamepad_button(struct sc_gamepad_processor *gp,
                                const struct sc_gamepad_button_event *event) {
    struct sc_gamepad_uhid *gamepad = DOWNCAST(gp);

    struct sc_hid_input hid_input;
    if (!sc_hid_gamepad_generate_input_from_button(&gamepad->hid, &hid_input,
                                                   event)) {
        return;
    }

    sc_gamepad_uhid_send_input(gamepad, &hid_input, "gamepad button");

}

void
sc_gamepad_uhid_process_hid_output(struct sc_gamepad_uhid *gamepad,
                                   uint16_t hid_id, const uint8_t *data,
                                   size_t size) {
    (void) gamepad;
    char *hex = sc_str_to_hex_string(data, size);
    if (hex) {
        LOGI("==== HID output [%" PRIu16 "] %s", hid_id, hex);
        free(hex);
    } else {
        LOGI("==== HID output [%" PRIu16 "]", hid_id);
    }

    struct sc_hid_gamepad *gp = &gamepad->hid;
    struct sc_hid_gamepad_slot* slot = &gp->slots[hid_id - SC_HID_ID_GAMEPAD_FIRST];
    SDL_GameController *gc = SDL_GameControllerFromInstanceID(slot->gamepad_id);
    // |           | HID Report | SDL Parameter |
    // |-----------|------------|---------------|
    // | Intensity | 0 - 100    | 0 - 65535     |
    // | Duration  | 0 - 255    | 0 - 1000      |
    SDL_GameControllerRumble(gc, data[4] * 0xFFFF / 100, data[5] * 0xFFFF / 100, data[6] * 1000 / 0xFF);
}

void
sc_gamepad_uhid_init(struct sc_gamepad_uhid *gamepad,
                     struct sc_controller *controller) {
    sc_hid_gamepad_init(&gamepad->hid);

    gamepad->controller = controller;

    static const struct sc_gamepad_processor_ops ops = {
        .process_gamepad_device = sc_gamepad_processor_process_gamepad_device,
        .process_gamepad_axis = sc_gamepad_processor_process_gamepad_axis,
        .process_gamepad_button = sc_gamepad_processor_process_gamepad_button,
    };

    gamepad->gamepad_processor.ops = &ops;
}
