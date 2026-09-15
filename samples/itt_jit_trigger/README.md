# PoC CWE-427 (JIT Profiling Intel ITT no OpenCV).

Este documento descreve detalhadamente tudo o que foi implementado, como os objetos e binários são gerados, como o teste determinístico funciona e como executar a validação da vulnerabilidade **CWE-427** (*Uncontrolled Search Path Element* / carregamento de DLL não confiável via variável de ambiente) no módulo `jitprofiling.c` do OpenCV.

---

## 1. Resumo do Vetor de Vulnerabilidade (CWE-427)

### Localização no Código do OpenCV
* **Arquivo **: `3rdparty/ittnotify/src/ittnotify/jitprofiling.c`
* **Função **: `loadiJIT_Funcs()` (chamada internamente por `iJIT_NotifyEvent` e `iJIT_IsProfilingActive`)

### Causa Raiz
No Windows (64-bit), a macro `NEW_DLL_ENVIRONMENT_VAR` é definida como `"INTEL_JIT_PROFILER64"` (e `"INTEL_JIT_PROFILER32"` em 32-bit):
```c
dNameLength = GetEnvironmentVariableA(NEW_DLL_ENVIRONMENT_VAR, NULL, 0);
if (dNameLength) {
    ...
    envret = GetEnvironmentVariableA(NEW_DLL_ENVIRONMENT_VAR, dllName, dNameLength);
    if (envret && isValidAbsolutePath(dllName, dNameLength)) {
        m_libHandle = LoadLibraryExA(dllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
}
```
A função `isValidAbsolutePath()` apenas verifica se a string começa com `X:\` ou `\\`. Ela **não restringe**:
1. Quem é o proprietário do arquivo ou as permissões do diretório.
2. Se o caminho pertence a uma pasta do sistema confiável (ex: `System32` ou pasta oficial do Intel VTune).
3. Se a DLL é assinada digitalmente.

Consequentemente, qualquer usuário ou processo com privilégios locais pode definir a variável de ambiente `INTEL_JIT_PROFILER64` apontando para qualquer DLL arbitrária. Assim que qualquer evento de profiling JIT for acionado, o processo carrega a biblioteca via `LoadLibraryExA()`, executando imediatamente o código de inicialização (`DllMain` / `DLL_PROCESS_ATTACH`) no contexto do processo da aplicação.

---

## 2. Mapa dos Binários e Bibliotecas Compiladas do OpenCV

Para compilar o sample e gerar o executável de teste sem depender de instalação global, foram utilizados os artefatos já compilados no diretório `build` do repositório:

| Arquivo Gerado | Caminho Completo | Papel no Processo |
| :--- | :--- | :--- |
| **`opencv_core510.dll`** | `build\bin\Release\opencv_core510.dll` | Biblioteca de execução em runtime do OpenCV Core |
| **`opencv_core510.lib`** | `build\lib\Release\opencv_core510.lib` | Biblioteca de importação usada pelo Linker (`link.exe`) |
| **`ittnotify.lib`** | `build\3rdparty\lib\Release\ittnotify.lib` | Biblioteca estática que contém `iJIT_NotifyEvent`, `loadiJIT_Funcs` e `iJIT_IsProfilingActive` |
| **`jitprofiling.h`** | `3rdparty\ittnotify\include\jitprofiling.h` | Header com declarações dos eventos, structs e protótipos |
| **Headers OpenCV** | `include\` e `modules\core\include\` | Headers públicos do OpenCV |
| **Config Módulos** | `build\opencv2\opencv_modules.hpp` | Header gerado pelo CMake contendo as diretivas dos módulos ativos |

---

## 3. Arquivos Adicionados ao Projeto

Todos os arquivos do sample foram organizados no diretório [`samples/itt_jit_trigger/`](file:///c:/Users/marco/source/repos/opencv/samples/itt_jit_trigger):

```
opencv/
├── .vscode/
│   ├── c_cpp_properties.json     # Mapeamento de includes e IntelliSense do OpenCV/ITT no VS Code
│   └── tasks.json                # Tarefas de Build (Ctrl+Shift+B) e Teste integradas ao VS Code
└── samples/
    └── itt_jit_trigger/
        ├── itt_jit_trigger.cpp       # Código C++ do executável de teste com múltiplos eventos JIT
        ├── poc_dll.c                 # Código C da biblioteca de prova (benigna)
        ├── build_sample.bat          # Script de compilação MSVC x64 de todos os objetos
        ├── run_test.bat              # Script de teste (Baseline vs Injeção)
        ├── CMakeLists.txt            # Arquivo de configuração CMake (opção alternativa)
        ├── README.md                 # Este documento explicativo
        ├── itt_jit_trigger.exe       # Binário executável gerado
        ├── poc_dll.dll               # DLL de prova gerada
        ├── opencv_core510.dll        # Runtime copiado para viabilizar execução direta
        └── POC_DLL_INJECTION_PROOF.txt # Prova material gerada ao carregar a DLL
```


---

## 4. Como Funcionam os Componentes do Teste

### 4.1. O Executável: `itt_jit_trigger.cpp`
Diferente de testes heurísticos que tentam acionar profiling indiretamente através de filtros de imagem, este sample chama **diretamente** as APIs públicas do ITT JIT. Isso garante 100% de reproducibilidade:

* **Evento 1 (`METHOD_LOAD_FINISHED` com `iJIT_Method_Load`)**:
  - Primeira chamada que aciona o `loadiJIT_Funcs()`.
  - É aqui que a variável `INTEL_JIT_PROFILER64` é lida e a DLL externa é carregada.
* **Evento 2 (`METHOD_LOAD_FINISHED` com dados adicionais)**:
  - Testa a passagem de dados de método (endereço de carga fictício `0x1000`, tamanho `128`).
  - Confirma que chamadas subsequentes reutilizam o handle já carregado (`static int bDllWasLoaded = 1`).
* **Evento 3 (`METHOD_LOAD_FINISHED_V2`)**:
  - Testa o registro de módulos dinâmicos (suporte a nomes de módulo customizados).
* **Evento 4 (`METHOD_UNLOAD_START`)**:
  - Testa o evento de descarregamento de método em tempo de execução.
* **Evento 5 (`iJIT_IsProfilingActive()`)**:
  - Consulta a flag de status retornada pela função `Initialize()` da biblioteca de profiling.
* **Evento 6 (`SHUTDOWN`)**:
  - Notifica o encerramento do agente de profiling.
* **Instanciação `cv::Mat`**:
  - Garante o vínculo efetivo com o runtime `opencv_core510.dll`.

### 4.2. A Biblioteca de Prova: `poc_dll.c`
Uma biblioteca benigna que comprova a injeção sem riscos de segurança:
* Em **`DllMain(DLL_PROCESS_ATTACH)`** (Windows) ou **`__attribute__((constructor))`** (Linux):
  - Ao ser carregada na memória do processo via `LoadLibraryExA`, escreve uma mensagem com timestamp no arquivo `POC_DLL_INJECTION_PROOF.txt`.
* Exporta **`Initialize()`**:
  - Chamada pelo `loadiJIT_Funcs()` logo após a carga da DLL.
  - Retorna `1` (`iJIT_SAMPLING_ON`), ativando o profiling no executável.
* Exporta **`NotifyEvent()`**:
  - Recebe os eventos JIT disparados e retorna `1` (sucesso).

---

## 5. Como os Objetos e Binários São Gerados

O processo de compilação transforma os fontes em código de máquina de 64 bits utilizando o compilador da Microsoft (`cl.exe`) e o vinculador (`link.exe`).

### 5.1. Compilação Automatizada
Basta executar o script:
```cmd
cd c:\Users\marco\source\repos\opencv\samples\itt_jit_trigger
build_sample.bat
```

### 5.2. Etapas Detalhadas da Compilação

#### Passo 1: Inicialização das Variáveis do MSVC (x64)
```cmd
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
```

#### Passo 2: Geração da DLL de Prova (`poc_dll.dll`)
* **Comando executado**:
  ```cmd
  cl /LD /MD /O2 poc_dll.c /Fe:poc_dll.dll
  ```
* **Objetos gerados**:
  - `poc_dll.obj`: Objeto intermediário compilado a partir do C.
  - `poc_dll.lib`: Arquivo de exportação de símbolos.
  - `poc_dll.exp`: Export table gerada pelo linker.
  - `poc_dll.dll`: A biblioteca dinâmica de prova.

#### Passo 3: Geração do Executável (`itt_jit_trigger.exe`)
* **Comando executado**:
  ```cmd
  cl /EHsc /MD /O2 itt_jit_trigger.cpp ^
     /I"..\..\include" ^
     /I"..\..\modules\core\include" ^
     /I"..\..\build" ^
     /I"..\..\3rdparty\ittnotify\include" ^
     /link ^
     /LIBPATH:"..\..\build\lib\Release" ^
     /LIBPATH:"..\..\build\3rdparty\lib\Release" ^
     opencv_core510.lib ittnotify.lib ^
     /OUT:itt_jit_trigger.exe
  ```
* **Objetos gerados**:
  - `itt_jit_trigger.obj`: Código objeto C++ compilado.
  - `itt_jit_trigger.exe`: O executável final vinculado a `opencv_core510.lib` e `ittnotify.lib`.

#### Passo 4: Cópia do Runtime do OpenCV
* **Comando executado**:
  ```cmd
  copy /y ..\..\build\bin\Release\opencv_core510.dll .
  ```
  *(Permite que `itt_jit_trigger.exe` seja executado diretamente em qualquer terminal sem necessidade de modificar a variável de sistema `%PATH%`)*.

### 5.3. Como Compilar e Executar Diretamente no Visual Studio Code (VS Code)

O repositório já está 100% configurado para o VS Code através da pasta `.vscode/`:

#### A. Compilação com Atalho de Teclado (Build Task)
1. Pressione **`Ctrl + Shift + B`** (ou acesse o menu superior **Terminal** -> **Run Build Task...**).
2. O VS Code executará automaticamente a tarefa padrão `"Compilar ITT JIT Sample (MSVC x64)"` configurada em `.vscode/tasks.json`.
3. A compilação rodará no terminal integrado exibindo a compilação de `poc_dll.dll`, `itt_jit_trigger.exe` e a cópia da DLL do OpenCV.

#### B. Pelo Terminal Integrado do VS Code (PowerShell ou CMD)
Basta abrir o terminal do VS Code (`Ctrl + \`` ou menu **Terminal** -> **New Terminal**) e executar:
```powershell
# Para compilar:
.\samples\itt_jit_trigger\build_sample.bat

# Para rodar o teste (Baseline vs Injeção):
.\samples\itt_jit_trigger\run_test.bat
```

> **Por que funciona sem erros no terminal padrão do VS Code?**
> Um terminal padrão do PowerShell no Windows normalmente não carrega as variáveis de ambiente do compilador C++ (`cl.exe`). O script `build_sample.bat` detecta automaticamente a instalação do Visual Studio via utilitário `vswhere.exe` e chama `vcvars64.bat` de forma transparente.

#### C. Execução do Teste pelo Menu de Tarefas do VS Code
1. Pressione **`Ctrl + Shift + P`** -> digite **`Tasks: Run Task`**.
2. Selecione **`Executar Teste ITT PoC (Baseline vs Injetado)`**.

#### D. Suporte Completo ao IntelliSense (`.vscode/c_cpp_properties.json`)
O arquivo de configuração do C/C++ mapeia todos os diretórios de include:
* `${workspaceFolder}/include`
* `${workspaceFolder}/modules/core/include`
* `${workspaceFolder}/3rdparty/ittnotify/include`
* `${workspaceFolder}/build`
Isso garante que, ao abrir `itt_jit_trigger.cpp`, o VS Code reconhece os tipos `cv::Mat`, `iJIT_Method_Load`, macros e protótipos de função sem erros de sintaxe ou sublinhados vermelhos.

---

## 6. Como Executar e Validar o Teste


Para rodar os testes, use o script `run_test.bat`:
```cmd
cd c:\Users\marco\source\repos\opencv\samples\itt_jit_trigger
run_test.bat
```

## 7. Debugging.
1. **No Teste 1 (Baseline)**:
   - A variável `INTEL_JIT_PROFILER64` não está definida.
   - `loadiJIT_Funcs()` falha silenciosamente ao não encontrar o agente.
   - Todos os eventos retornam `0`.
   - O arquivo `POC_DLL_INJECTION_PROOF.txt` **não existe**.

2. **No Teste 2 (Injeção via Variável de Ambiente)**:
   - A variável `INTEL_JIT_PROFILER64` aponta para o caminho da DLL (`poc_dll.dll`).
   - `isValidAbsolutePath()` valida apenas o prefixo de drive (`C:\`).
   - `LoadLibraryExA()` carrega a DLL no processo do executável.
   - O sistema operacional chama `DllMain` com a razão `DLL_PROCESS_ATTACH`, disparando o primeiro log.
   - O `loadiJIT_Funcs()` obtém o ponteiro de `Initialize()` e o chama, disparando o segundo log.
   - O status `iJIT_IsProfilingActive()` muda para `1` (`iJIT_SAMPLING_ON`).
   - Todos os retornos passam a ser `1` (sucesso).
   - O arquivo de auditoria `POC_DLL_INJECTION_PROOF.txt` é gerado, comprovando a vulnerabilidade.

---

## 8. Critérios para Validação da Correção (Patch)

Quando um patch de correção for aplicado em `3rdparty/ittnotify/src/ittnotify/jitprofiling.c` (ex: validação com allowlist de diretórios seguros como `Program Files\Intel...`, verificação de integridade ou desabilitação de carregamento dinâmico não seguro por padrão):

| Cenário de Teste | Antes da Correção (Vulnerável) | Após a Correção (Seguro) |
| :--- | :--- | :--- |
| `INTEL_JIT_PROFILER64=C:\qualquer\poc_dll.dll` | DLL carregada e executada (Retorno 1, arquivo criado) | **Bloqueado** (Retorno 0, nenhum arquivo criado) |
| Caminho relativo (`.\poc_dll.dll`) | Pode carregar dependendo do diretório de trabalho | **Bloqueado** |
| Variável não definida | Retorno 0, comportamento padrão | Retorno 0, comportamento padrão |
| DLL legítima assinada em diretório do sistema | Carrega normalmente | Carrega normalmente |
