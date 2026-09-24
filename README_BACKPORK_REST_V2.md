# BackPork nativo: teste experimental de montagem antecipada

Base: `fix/ppr-restmode-v2`, que passou nos testes de repouso relatados pelo usuário. Esta ramificação gera **um ELF** com o monitor BackPork compilado em `ps5-kstuff-ldr/src/backpork.c`; não inclui um segundo ELF BackPork.

## Motivo

Em `putty.log` de 2026-09-23 20:15, `PPSA28180` apresentou `PRX_NOT_RESOLVED_FUNCTION` antes da linha `[BP] mounted` em duas tentativas. A versão antiga também aguardava a remoção do sandbox antes de desmontar, enquanto o sistema registrava falha ao remover `common/lib` ainda montado.

## Alterações

- O monitor cria um worker por `NOTE_CHILD`, em vez de esperar por `NOTE_EXEC` no loop principal.
- Cada worker registra `NOTE_EXIT` antes da tentativa de montagem, consulta o título e tenta montar assim que `app0/fakelib` e `common/lib` existirem. O limite é de 250 tentativas com pausa de 2 ms.
- Ao receber `NOTE_EXIT`, desmonta imediatamente. Não remove diretórios do sandbox.
- Se `SceSysCore.elf` sair, o monitor procura uma nova instância e registra o evento novamente.
- BackPork escreve somente no klog. Permanece apenas a notificação gráfica do kstuff.

## Validação

Build com SDK PS5 no Docker: sucesso, `-Wall -Werror`. ELF PIE x86-64 gerado; SHA-256 `7bf03f4437fd2b7da77b10a73fd8e31b73b0cc306eae71a9ae3006cc24a2002f`.

**Ainda não validado no PS5.** O ponto decisivo no klog é `[BP] mounted early` antes de qualquer `PRX_NOT_RESOLVED_FUNCTION`, seguido de `[BP] unmount ... rc=0` após fechar o jogo. Se `early mount missed` aparecer, o monitor não conseguiu montar a tempo. Faça o primeiro teste em boot limpo, com um único envio do ELF, e capture o klog desde o envio até o fechamento do jogo. O resultado de reinício e repouso desta integração permanece desconhecido.

BackPork: [BestPig/BackPork](https://github.com/BestPig/BackPork), GPL-3.0. Kstuff: [EchoStretch/kstuff-lite](https://github.com/EchoStretch/kstuff-lite).
