## 8. Arquitetura do Core

O core do projeto HyperVLM foi projetado como um núcleo computacional independente, responsável por análise, transformação e execução de programas, sem dependência de interfaces gráficas ou motores externos.

### 8.1 Visão Geral da Arquitetura

A arquitetura segue uma separação em camadas bem definidas:

- Entrada textual (programa fonte)
- Análise léxica
- Análise sintática
- Representação intermediária (IR)
- Máquina Virtual (VM)
- Ambiente de execução (DRE)

Cada camada comunica-se apenas com a camada adjacente, evitando acoplamento indevido.

### 8.2 Módulos Principais

- **Lexer**: converte texto em tokens
- **Parser**: constrói a árvore sintática
- **IRBuilder**: gera a representação intermediária
- **VM**: executa o programa de forma determinística
- **DRE**: gerencia o ambiente de execução

### 8.3 Isolamento Arquitetural

O core pode ser compilado e executado isoladamente, como demonstrado nos testes experimentais, garantindo sua reutilização e extensibilidade.

### 8.4 Extensibilidade

Novos módulos podem ser integrados ao core sem alteração das invariantes fundamentais, preservando corretude e determinismo.
