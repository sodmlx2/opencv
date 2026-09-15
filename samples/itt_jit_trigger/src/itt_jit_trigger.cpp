#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstring>

#include <jitprofiling.h>

int main() {
    std::cout << "=== Disparando eventos ITT/JIT Profiling ===" << std::endl;

    // 1. Notifica registro de metodo inicial.
    //    Esta chamada aciona loadiJIT_Funcs(), que tenta carregar a DLL
    //    apontada pela variavel de ambiente (ex: INTEL_JIT_PROFILER64 no Windows).
    iJIT_Method_Load initMethod;
    std::memset(&initMethod, 0, sizeof(iJIT_Method_Load));
    initMethod.method_id = 1;
    initMethod.method_name = (char*)"MetodoInicial_Startup";

    int isProfilingActive = iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, &initMethod);
    std::cout << "[1] METHOD_LOAD_FINISHED disparado. Retorno: " << isProfilingActive << std::endl;

    // 2. Simula registro de um metodo ficticio sendo carregado com dados completos.
    iJIT_Method_Load methodData;
    std::memset(&methodData, 0, sizeof(iJIT_Method_Load));
    methodData.method_id = 2;
    methodData.method_name = (char*)"MetodoFicticio_Preprocessamento";
    methodData.method_load_address = (void*)0x1000;
    methodData.method_size = 128;
    methodData.line_number_size = 0;

    int ret2 = iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, &methodData);
    std::cout << "[2] METHOD_LOAD_FINISHED (com dados) disparado. Retorno: " << ret2 << std::endl;

    // 3. Notifica inicio de um modulo carregado (V2 da API).
    iJIT_Method_Load_V2 methodDataV2;
    std::memset(&methodDataV2, 0, sizeof(iJIT_Method_Load_V2));
    methodDataV2.method_id = 3;
    methodDataV2.method_name = (char*)"MetodoFicticio_Filtro";
    methodDataV2.module_name = (char*)"ModuloOpenCVCustom";

    int ret3 = iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED_V2, &methodDataV2);
    std::cout << "[3] METHOD_LOAD_FINISHED_V2 disparado. Retorno: " << ret3 << std::endl;

    // 4. Notifica descarregamento do metodo (evento de unload).
    unsigned int methodId = 2;
    int ret4 = iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_UNLOAD_START, &methodId);
    std::cout << "[4] METHOD_UNLOAD_START disparado. Retorno: " << ret4 << std::endl;

    // 5. Consulta se o profiling esta de fato ativo (API auxiliar,
    //    tambem depende da DLL ter sido carregada com sucesso).
    iJIT_IsProfilingActiveFlags flag = iJIT_IsProfilingActive();
    std::cout << "[5] iJIT_IsProfilingActive() retornou: " << flag << std::endl;

    // 6. Notifica encerramento (SHUTDOWN)
    int ret6 = iJIT_NotifyEvent(iJVM_EVENT_TYPE_SHUTDOWN, NULL);
    std::cout << "[6] SHUTDOWN disparado. Retorno: " << ret6 << std::endl;

    std::cout << "=== Fim dos disparos. Verifique se a lib externa foi carregada. ===" << std::endl;

    // Uso minimo do OpenCV so pra manter o link com a lib instrumentada
    cv::Mat img = cv::Mat::zeros(10, 10, CV_8UC1);
    std::cout << "OpenCV Mat criado com sucesso: " << img.size() << std::endl;

    return 0;
}
