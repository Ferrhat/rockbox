#include "plugin.h"
#include <stdlib.h>

#define CONF_PATH "/system/etc/audio_policy.conf"
#define CONF_SPEAKER_ON "/sdcard/.rockbox/audio_policy_speaker.conf"
#define CONF_SPEAKER_OFF "/sdcard/.rockbox/audio_policy_nospeaker.conf"
#define SPEAKER_TOKEN "AUDIO_DEVICE_OUT_SPEAKER"

static int get_speaker_state(void)
{
    int fd = rb->open(CONF_PATH, O_RDONLY, 0);
    if (fd < 0) return -1;
    char buf[4096];
    ssize_t n = rb->read(fd, buf, sizeof(buf) - 1);
    rb->close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    char *p = buf;
    while (*p) {
        if (rb->strncmp(p, SPEAKER_TOKEN, sizeof(SPEAKER_TOKEN) - 1) == 0)
            return 1;
        p++;
    }
    return 0;
}

static void apply_state(bool enable)
{
    const char *src = enable ? CONF_SPEAKER_ON : CONF_SPEAKER_OFF;
    char cmd[256];
    rb->snprintf(cmd, sizeof(cmd),
        "su -c 'mount -o rw,remount /system"
        " && cp \"%s\" " CONF_PATH
        " && chmod 644 " CONF_PATH
        " && mount -o ro,remount /system'", src);
    system(cmd);
}

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    int state = get_speaker_state();
    if (state < 0) { rb->splash(HZ * 2, "Error reading config."); return PLUGIN_OK; }
    bool speaker_on = (state == 1);
    char msg[64];
    rb->snprintf(msg, sizeof(msg), "Speaker: %s\nOK=toggle BACK=cancel", speaker_on ? "ON" : "OFF");
    rb->splash(0, msg);
    int btn;
    while (true) {
        btn = rb->get_action(CONTEXT_STD, HZ * 15);
        if (btn == ACTION_STD_OK) break;
        if (btn == ACTION_STD_CANCEL || btn == ACTION_STD_PREV) {
            rb->splash(HZ, "Cancelled."); return PLUGIN_OK;
        }
    }
    rb->splash(0, speaker_on ? "Disabling..." : "Enabling...");
    apply_state(!speaker_on);
    rb->sleep(HZ);
    rb->splash(0, speaker_on ? "Disabled.\nOK=reboot BACK=later" : "Enabled.\nOK=reboot BACK=later");
    while (true) {
        btn = rb->get_action(CONTEXT_STD, HZ * 15);
        if (btn == ACTION_STD_OK) { rb->sys_reboot(); break; }
        if (btn == ACTION_STD_CANCEL || btn == ACTION_STD_PREV) break;
    }
    return PLUGIN_OK;
}
