7. Modelo de Estados da Máquina Virtual

A Máquina Virtual (VM) do projeto HyperVLM é responsável pela execução determinística de programas compilados para a representação intermediária do sistema. Seu funcionamento é descrito por um modelo explícito de estados, no qual cada transição é controlada e observável.

Esse modelo garante previsibilidade, isolamento de falhas e separação clara entre as fases de compilação e execução.

7.1 Visão Geral

A VM opera como uma máquina de estados finitos, na qual apenas transições válidas são permitidas. Cada estado representa uma fase bem definida do ciclo de vida de execução de um programa.

O comportamento observado experimentalmente — incluindo carregamento do artefato .vlm e execução passo a passo — corresponde diretamente a esse modelo formal.

7.2 Estados da VM

A seguir estão os estados formais definidos para a Máquina Virtual:

INIT
Estado inicial da VM. Nenhum programa foi carregado.

LOAD
Estado em que o artefato intermediário (.vlm) é carregado e validado.

READY
Programa carregado com sucesso. A VM está pronta para iniciar a execução.

STEP
Execução incremental de uma instrução ou bloco lógico. Esse estado pode ocorrer repetidamente.

HALT
Estado final de execução normal. O programa terminou sem erros.

ERROR
Estado terminal atingido quando ocorre erro sintático, semântico ou violação de invariante.

7.3 Diagrama de Estados
        +------+
        | INIT |
        +------+
            |
            v
        +------+
        | LOAD |
        +------+
            |
      +-----+-----+
      |           |
      v           v
  +--------+   +--------+
  | READY  |   | ERROR  |
  +--------+   +--------+
      |
      v
  +--------+
  | STEP   |<------+
  +--------+       |
      |            |
      v            |
  +--------+       |
  | HALT   |-------+
  +--------+

7.4 Transições de Estado
Estado Atual	Evento	Próximo Estado
INIT	Inicialização da VM	LOAD
LOAD	Artefato válido carregado	READY
LOAD	Erro de leitura ou validação	ERROR
READY	Início da execução	STEP
STEP	Instrução válida executada	STEP
STEP	Fim do programa	HALT
STEP	Erro de execução	ERROR
7.5 Propriedades do Modelo

O modelo de estados da VM garante as seguintes propriedades:

Determinismo: a execução segue sempre a mesma sequência de estados para entradas equivalentes.

Execução incremental: a VM avança por passos discretos, facilitando inspeção e controle.

Isolamento de falhas: qualquer erro leva a um estado terminal seguro.

Separação de fases: compilação e execução não se sobrepõem no modelo de estados.

7.6 Relação com a Validação Experimental

Os testes realizados confirmaram empiricamente esse modelo. Em particular:

A mensagem [VM] loading corresponde à transição INIT → LOAD.

As mensagens [VM] step correspondem às iterações no estado STEP.

A ausência de falhas confirma transições válidas até HALT.

Erros de parsing conduzem diretamente ao estado ERROR.

Assim, o modelo teórico apresentado está em conformidade com o comportamento observado do sistema.

7.7 Considerações Finais

O modelo de estados da Máquina Virtual fornece uma base formal para o entendimento da execução do core do HyperVLM, sendo essencial para análise de corretude, extensibilidade futura e justificativa de propriedades como determinismo e robustez.
