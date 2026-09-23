# kstuff + A53/PPR

Integração em código-fonte do carregador **kstuff-lite** com a instalação dos patches **PPR/A53**. O resultado da compilação é um único `kstuff.elf`: a rotina A53/PPR é executada no início de `ps5-kstuff-ldr/src/main.c` e, após sucesso, o carregador inicia o kstuff. Não há um segundo ELF anexado ou executado internamente.

Este repositório contém **somente kstuff + A53/PPR**. BackPork não faz parte desta versão.

## Estado dos testes

- **PS5 firmware 5.00:** integração e inicialização do kstuff testadas no console.
- Um teste manual mediu aproximadamente **4,54 s** entre enviar o ELF pelo loader e o kstuff iniciar. O tempo varia conforme o ambiente; não é uma garantia de desempenho.
- Existem perfis adicionais em [`ps5-kstuff-ldr/src/ppr/ppr_profiles.inc`](ps5-kstuff-ldr/src/ppr/ppr_profiles.inc). A presença de um perfil no código **não** significa que esta integração tenha sido testada nesse firmware.
- Uma compilação bem-sucedida confirma apenas que o código gerou o ELF; o funcionamento precisa ser validado no console correspondente.

## Compilação

Requisitos: Linux ou contêiner Linux, `make`, `git` e [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk). Os submódulos Git precisam estar presentes.

```bash
git clone --recurse-submodules https://github.com/EliasSamuca/kstuff-a53.git
cd kstuff-a53
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
./ci-ps5-kstuff-ldr.sh
```

O arquivo gerado fica em `ps5-kstuff-ldr/kstuff.elf`. Se o SDK estiver em outro local, ajuste `PS5_PAYLOAD_SDK`. Para entrar no contêiner usado durante o desenvolvimento pelo PowerShell: `docker exec -it ps5-dev bash`.

## Sequência de execução

1. O loader inicializa a comunicação A53 e verifica o perfil PPR correspondente ao firmware e ao tipo de console.
2. Instala e verifica os patches PPR. Uma falha interrompe a inicialização do kstuff.
3. Carrega o payload do kstuff e aplica as etapas normais do loader.

Os tempos de cada etapa podem ser observados no klog com prefixo `[TIME]` e, quando disponível, em `/data/kstuff-startup.log`.

## ELF testado no FW 5.00

O arquivo usado no teste do console está em [`artifacts/fw5/kstuff-a53-fast-klog-experimental.elf`](artifacts/fw5/kstuff-a53-fast-klog-experimental.elf). Confira o hash SHA-256 e a diferença entre essa versão e a branch `main` em [`artifacts/fw5/README.md`](artifacts/fw5/README.md).

## Origens e créditos

- [EchoStretch/kstuff-lite](https://github.com/EchoStretch/kstuff-lite): projeto base e histórico Git preservado neste fork; base usada: `2506a5b15af501734f7ea3dce2759c077cdb76c6`.
- [drakmor/ppr-patch](https://github.com/drakmor/ppr-patch): referência e código da instalação dos patches PPR, adaptados para a inicialização pelo loader. Consulte os avisos de licença desse projeto.
- [cragson/a53-code-exec](https://github.com/cragson/a53-code-exec): referência técnica para a pesquisa A53; o PoC original não é empacotado como um ELF adicional aqui.
- [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk): ferramenta de compilação, instalada separadamente.

Detalhes da atribuição estão em [`CREDITS.md`](CREDITS.md). A documentação técnica da base original está preservada em [`docs/KSTUFF_UPSTREAM.md`](docs/KSTUFF_UPSTREAM.md). Mantenha os avisos de copyright e licença presentes nos arquivos de origem e nos submódulos.

## Observações

Este projeto é experimental e destinado à pesquisa e a testes em consoles próprios. Antes de testar outra versão de firmware, confirme o perfil exato no código e faça validação específica nesse console.
