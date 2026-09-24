# kstuff + A53/PPR + BackPork

Payload experimental para PS5 **firmware 5.00**. O carregador instala/verifica os patches A53/PPR, inicia o kstuff e executa o monitor BackPork no mesmo processo. A compilação produz **um único ELF**: o BackPork é integrado em código-fonte, sem anexar outro ELF, fazer `fork` ou exigir um segundo envio.

## Download

**[Baixar a v8 experimental com BackPork](https://github.com/EliasSamuca/kstuff-a53/releases/download/fw5-backpork-v8/kstuff-a53-backpork-RESUME-v8-FIXED-EXPERIMENTAL.elf)** · [Notas da versão e código-fonte](https://github.com/EliasSamuca/kstuff-a53/releases/tag/fw5-backpork-v8)

SHA-256: `3713495c9c4fe4ff819507e0b9c3490c7694adbe97dbaef8dc363339497da0f5`.

A [versão anterior, somente kstuff+A53/PPR](https://github.com/EliasSamuca/kstuff-a53/releases/tag/fw5-tested-source), continua disponível separadamente. Ela não contém BackPork.

## Funcionamento

1. O loader verifica o perfil exato do console e instala os patches PPR/A53. Se falhar, não inicia o kstuff.
2. O kstuff é carregado normalmente. Um monitor BackPork nativo observa os processos criados pelo `SceSysCore`.
3. Para jogos com `app0/fakelib`, o monitor monta essa pasta sobre `common/lib` antes do `EXEC` do jogo e desmonta quando o processo termina. Um bloqueio evita dois monitores desta integração na mesma sessão.
4. Ao voltar do repouso, o monitor registra novamente o `SceSysCore`. A notificação **“BackPork active again!”** só aparece após a primeira montagem antecipada bem-sucedida depois do retorno. Ela confirma a montagem, não garante que todo jogo funcionará.

O arquivo principal é [`ps5-kstuff-ldr/src/backpork.c`](ps5-kstuff-ldr/src/backpork.c). A rotina de energia fica em [`ps5-kstuff-ldr/src/ppr/ppr_resume.c`](ps5-kstuff-ldr/src/ppr/ppr_resume.c). A v8 corrigiu um acesso ao nome da pasta do sandbox depois de `closedir`, que na v7 podia produzir um caminho corrompido e falhar na primeira abertura. Veja [detalhes e logs dos testes](README_BACKPORK_DIRENT_V8.md).

## Estado dos testes

- FW 5.00: no log da v8 **após repouso**, `PPSA17221` e `PPSA28180` montaram `fakelib` antes do `EXEC` na primeira tentativa e desmontaram com `rc=0`. O monitor foi rearmado e o PPR foi verificado antes dos lançamentos.
- Na v7, uma primeira abertura após boot limpo falhou por um caminho de montagem corrompido. A v8 corrige a causa no código e compilou sem erros, mas **o log pré-repouso da v8 ainda não pôde ser lido aqui**. A primeira abertura após boot limpo ainda precisa dessa confirmação específica.
- Outros jogos, outros firmwares, ciclos prolongados de repouso e reinício com BackPork integrado não estão validados. Perfis adicionais no código não equivalem a testes nesses firmwares.

Não carregue outro BackPork junto deste ELF. Os logs relevantes usam os prefixos `[TIME]`, `[PPR]` e `[BP]` no klog. `mounted before exec` e `unmount ... rc=0` são os sinais principais para a montagem do jogo.

## Compilação

Requisitos: Linux ou contêiner Linux, `make`, `git`, [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk) e os submódulos Git do projeto.

```bash
git clone --recurse-submodules https://github.com/EliasSamuca/kstuff-a53.git
cd kstuff-a53
git checkout fw5-backpork-v8
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
./ci-ps5-kstuff-ldr.sh
```

O resultado fica em `ps5-kstuff-ldr/kstuff.elf`. Ajuste `PS5_PAYLOAD_SDK` se o SDK estiver em outro local. O hash acima identifica o ELF disponibilizado no pré-lançamento; compilações locais podem gerar hash diferente.

## Origens, créditos e licença

- [EchoStretch/kstuff-lite](https://github.com/EchoStretch/kstuff-lite): base do kstuff; histórico original preservado, commit-base `2506a5b15af501734f7ea3dce2759c077cdb76c6`.
- [drakmor/ppr-patch](https://github.com/drakmor/ppr-patch): base da instalação PPR/A53, adaptada ao loader.
- [BestPig/BackPork](https://github.com/BestPig/BackPork): base do monitor de jogos e da montagem `fakelib`, adaptada para o mesmo processo do loader.
- [cragson/a53-code-exec](https://github.com/cragson/a53-code-exec): referência técnica de A53; o PoC não é empacotado como ELF interno.
- [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk): ferramenta de compilação externa.

Veja [`CREDITS.md`](CREDITS.md), [`LICENSE`](LICENSE) e as licenças dos submódulos. O projeto destina-se a pesquisa e testes em consoles próprios.
