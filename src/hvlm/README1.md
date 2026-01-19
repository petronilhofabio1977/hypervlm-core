# HyperVLM Core

HyperVLM Core é um núcleo computacional próprio, desenvolvido com foco em arquitetura de sistemas, engenharia de linguagens e execução determinística.

Este repositório contém exclusivamente o **core fechado e funcional** do projeto HyperVLM.

---

## Objetivo do Projeto

O objetivo do HyperVLM Core é implementar, de forma prática e verificável, um pipeline completo de uma linguagem de programação, cobrindo todas as etapas fundamentais:

* Leitura de código-fonte
* Análise sintática (AST)
* Representação intermediária (IR)
* Lowering para formato executável próprio
* Geração de binário dedicado
* Execução em máquina virtual

O projeto não é uma aplicação final, mas uma **plataforma base** para futuras extensões.

---

## Pipeline Implementado

Pipeline completo implementado e validado na prática:

```
Código Fonte (.hv)
  -> Parser / AST
  -> IR (Intermediate Representation)
  -> HVNodes (lowering)
  -> Binário VLM (.vlm)
  -> HyperVLM Virtual Machine
  -> Execução
```

Este fluxo foi compilado, executado e testado diretamente via terminal.

---

## Componentes do Core

### Frontend

* Lexer e Parser próprios
* Construção de AST
* Relato básico de erros de sintaxe

### IR (Intermediate Representation)

* Representação intermediária explícita
* Separação entre semântica e execução
* Base para análises e otimizações futuras

### Lowering

* Conversão do IR para nós executáveis (HVNodes)
* Geração de binário determinístico (.vlm)

### Máquina Virtual (VM)

* Carregamento de arquivos .vlm
* Execução passo a passo (step)
* Loop de execução funcional

---

## Compilação e Execução

O core não depende de CMake.
A compilação oficial é feita manualmente via `g++`.

### Compilar o core

```
g++ -std=c++20 \
  -Iinclude \
  -Isrc \
  tools/hvlmc_core.cpp \
  src/hvlm/vm/*.cpp \
  src/hvlm/runtime/*.cpp \
  src/hvlm/core/*.cpp \
  -o hvlmc
```

### Executar um programa de exemplo

```
./hvlmc examples/hello.hv --run
```

### Saída típica

```
[VM] loading examples/hello.hv.vlm
[VM] step
[VM] step
```

Essa saída confirma que o pipeline completo foi executado com sucesso.

---

## Organização do Repositório

```
include/      headers do core
src/hvlm/     implementação do core (IR, VM, runtime)
tools/        ferramentas CLI (hvlmc)
examples/     exemplos de código-fonte (.hv)
```

Elementos fora do escopo do core (renderer, IA, experimentos, builds e binários gerados) não fazem parte deste repositório.

---

## Estado do Projeto

* Core computacional fechado
* Pipeline ponta-a-ponta funcional
* Execução comprovada em máquina virtual
* Repositório limpo e versionado

A partir do commit:

```
Close core pipeline: compile HV -> VLM -> VM execution
```

o core é considerado **estável e encerrado**.

---

## Licença

Este projeto é distribuído sob a licença definida no arquivo `LICENSE`.

---

## Autor

Fábio Petronilho de Oliveira

