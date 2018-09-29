#include "common.h"
#include "dynlib.h"
#include "m64p_common.h"
#include "m64p_frontend.h"
#include <stdio.h>

ptr_CoreStartup g_core_startup = NULL;
ptr_CoreShutdown g_core_shutdown = NULL;
ptr_CoreDoCommand g_core_do_command = NULL;
ptr_CoreAttachPlugin g_core_attach_plugin = NULL;
ptr_CoreDetachPlugin g_core_detach_plugin = NULL;

m64p_dynlib_handle g_core_handle            = NULL;
m64p_dynlib_handle g_plugin_video_handle    = NULL;
m64p_dynlib_handle g_plugin_audio_handle    = NULL;
m64p_dynlib_handle g_plugin_input_handle    = NULL;
m64p_dynlib_handle g_plugin_rsp_handle      = NULL;

m64p_rom_settings g_current_rom_settings;
m64p_rom_header g_current_rom_header;

boolean g_rom_settings_loaded = FALSE;
boolean g_rom_header_loaded = FALSE;
boolean g_verbose = FALSE;

void m64_set_verbose(boolean b)
{
    g_verbose = b;
}

void m64_debug_callback(void* context, int level, const char* message)
{
    if (level == M64MSG_ERROR) {
        printf("%s Error: %s\n", (const char *) context, message);
    } else if (level == M64MSG_WARNING) {
        printf("%s Warning: %s\n", (const char *) context, message);
    } else if (level == M64MSG_INFO) {
        printf("%s: %s\n", (const char *) context, message);
    } else if (level == M64MSG_STATUS) {
        printf("%s Status: %s\n", (const char *) context, message);
    } else if (level == M64MSG_VERBOSE)
    {
        if (g_verbose) {
            printf("%s: %s\n", (const char *) context, message);
        }
    }
    else {
        printf("%s Unknown: %s\n", (const char *) context, message);
    }
}

int m64_load_corelib(const char* path)
{
    m64p_error open_result = dynlib_open(&g_core_handle, path);

    if (open_result != M64ERR_SUCCESS) {
        return open_result;
    }

    g_core_startup = dynlib_getproc (g_core_handle, "CoreStartup");
    g_core_shutdown = dynlib_getproc(g_core_handle, "CoreShutdown");
    g_core_do_command = dynlib_getproc(g_core_handle, "CoreDoCommand");
    g_core_attach_plugin = dynlib_getproc(g_core_handle, "CoreAttachPlugin");
    g_core_detach_plugin = dynlib_getproc(g_core_handle, "CoreDetachPlugin");

    return M64ERR_SUCCESS;
}

int m64_start_corelib(char* config_path, char* data_path)
{
    return (*g_core_startup)(0x020001, config_path, data_path, "Core", m64_debug_callback, NULL, NULL);
}

int m64_shutdown_corelib()
{
    return (*g_core_shutdown)();
}

int m64_unload_corelib()
{
    if (g_core_handle == NULL) {
        return M64ERR_INVALID_STATE;
    }

    g_core_startup = NULL;

    dynlib_close (g_core_handle);

    g_core_handle = NULL;

    return M64ERR_SUCCESS;
}

int m64_load_plugin(m64p_plugin_type type, const char* path)
{
    if (path == NULL) {
        return M64ERR_INPUT_INVALID;
    }

    m64p_error err = M64ERR_SUCCESS;
    ptr_PluginStartup plugin_startup = NULL;

    switch (type)
    {
        case M64PLUGIN_RSP:
            if (g_plugin_rsp_handle != NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = dynlib_open(&g_plugin_rsp_handle, path);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to load RSP plugin: %s\n", path);
                return err;
            }

            plugin_startup = dynlib_getproc (g_plugin_rsp_handle, "PluginStartup");
            if (plugin_startup == NULL) {
                printf("Error: library '%s' broken.  No PluginStartup() function found.", path);
                return M64ERR_PLUGIN_FAIL;
            }

            err = (*plugin_startup)(g_core_handle, "RSP_PLUGIN", m64_debug_callback);
            if (err != M64ERR_SUCCESS) {
                printf("Error: RSP plugin library '%s' failed to start.", path);
                return err;
            }

            err = (*g_core_attach_plugin)(type, g_plugin_rsp_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to attach RSP plugin: %s\n", path);
                return err;
            }
            break;
        case M64PLUGIN_GFX:
            if (g_plugin_video_handle != NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = dynlib_open(&g_plugin_video_handle, path);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to load Video plugin: %s\n", path);
                return err;
            }

            plugin_startup = dynlib_getproc (g_plugin_video_handle, "PluginStartup");
            if (plugin_startup == NULL) {
                printf("Error: library '%s' broken.  No PluginStartup() function found.", path);
                return M64ERR_PLUGIN_FAIL;
            }

            err = (*plugin_startup)(g_core_handle, "GFX_PLUGIN", m64_debug_callback);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Video plugin library '%s' failed to start.", path);
                return err;
            }

            err = (*g_core_attach_plugin)(type, g_plugin_video_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to attach Video plugin: %s\n", path);
                return err;
            }
            break;
        case M64PLUGIN_AUDIO:
            if (g_plugin_audio_handle != NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = dynlib_open(&g_plugin_audio_handle, path);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to load Audio plugin: %s\n", path);
                return err;
            }

            plugin_startup = dynlib_getproc (g_plugin_audio_handle, "PluginStartup");
            if (plugin_startup == NULL) {
                printf("Error: library '%s' broken.  No PluginStartup() function found.", path);
                return M64ERR_PLUGIN_FAIL;
            }

            err = (*plugin_startup)(g_core_handle, "AUDIO_PLUGIN", m64_debug_callback);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Audio plugin library '%s' failed to start.", path);
                return err;
            }

            err = (*g_core_attach_plugin)(type, g_plugin_audio_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to attach Audio plugin: %s\n", path);
                return err;
            }
            break;
        case M64PLUGIN_INPUT:
            if (g_plugin_input_handle != NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = dynlib_open(&g_plugin_input_handle, path);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to load Input plugin: %s\n", path);
                return err;
            }

            plugin_startup = dynlib_getproc (g_plugin_input_handle, "PluginStartup");
            if (plugin_startup == NULL) {
                printf("Error: library '%s' broken.  No PluginStartup() function found.", path);
                return M64ERR_PLUGIN_FAIL;
            }

            err = (*plugin_startup)(g_core_handle, "INPUT_PLUGIN", m64_debug_callback);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Input plugin library '%s' failed to start.", path);
                return err;
            }

            err = (*g_core_attach_plugin)(type, g_plugin_input_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to attach Input plugin: %s\n", path);
                return err;
            }
            break;
        default:
            break;
    }

    return err;
}

int m64_unload_plugin(m64p_plugin_type type)
{
    m64p_error err = M64ERR_SUCCESS;

    switch (type)
    {
        case M64PLUGIN_RSP:
            if (g_plugin_rsp_handle == NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = (*g_core_detach_plugin)(type);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to detach RSP plugin.\n");
                return err;
            }

            err = dynlib_close(g_plugin_rsp_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to unload RSP plugin.\n");
                return err;
            }

            g_plugin_rsp_handle = NULL;
            break;
        case M64PLUGIN_GFX:
            if (g_plugin_video_handle == NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = (*g_core_detach_plugin)(type);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to detach Video plugin.\n");
                return err;
            }

            err = dynlib_close(g_plugin_video_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to unload Video plugin.\n");
                return err;
            }

            g_plugin_video_handle = NULL;
            break;
        case M64PLUGIN_AUDIO:
            if (g_plugin_audio_handle == NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = (*g_core_detach_plugin)(type);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to detach Audio plugin.\n");
                return err;
            }

            err = dynlib_close(g_plugin_audio_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to unload Audio plugin.\n");
                return err;
            }

            g_plugin_audio_handle = NULL;
            break;
        case M64PLUGIN_INPUT:
            if (g_plugin_input_handle == NULL) {
                return M64ERR_INVALID_STATE;
            }

            err = (*g_core_detach_plugin)(type);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to detach Input plugin.\n");
                return err;
            }

            err = dynlib_close(g_plugin_input_handle);
            if (err != M64ERR_SUCCESS) {
                printf("Error: Failed to unload Input plugin.\n");
                return err;
            }

            g_plugin_input_handle = NULL;
            break;
        default:
            break;
    }

    return err;
}

int m64_command(m64p_command command, int param_int, void* param_ptr)
{
    int retval = (*g_core_do_command)(command, param_int, param_ptr);

    switch (command)
    {
        case M64CMD_ROM_OPEN:
            if ((*g_core_do_command)(M64CMD_ROM_GET_SETTINGS, sizeof(m64p_rom_settings), &g_current_rom_settings) !=
                    M64ERR_SUCCESS)
            {
                printf("Error: Failed to load ROM settings.\n");
            }
            g_rom_settings_loaded = TRUE;
            if ((*g_core_do_command)(M64CMD_ROM_GET_HEADER, sizeof(m64p_rom_header), &g_current_rom_header) !=
                    M64ERR_SUCCESS)
            {
                printf("Error: Failed to load ROM header.\n");
            }
            g_rom_header_loaded = TRUE;
            break;
        case M64CMD_ROM_CLOSE:
            g_rom_settings_loaded = FALSE;
            g_rom_header_loaded = FALSE;
            break;
        default:
            break;
    }

    return retval;
}

char* m64_get_rom_goodname ()
{
    if (!g_rom_settings_loaded) {
        return NULL;
    }

    return g_current_rom_settings.goodname;
}