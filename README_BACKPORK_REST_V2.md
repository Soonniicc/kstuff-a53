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

## Resultado no console em 2026-09-23

**Esta versão NÃO é considerada estável. Não reutilizar como build de uso diário.**

- Teste 1 (`putty-test1.log`): o kstuff e um `backpork.elf` separado já estavam carregados. Os dois monitores montaram `fakelib` sobre o mesmo `common/lib`; o jogo abriu, mas o sistema registrou falhas de limpeza do sandbox após o fechamento. Portanto, este teste não valida a versão única isoladamente.
- Teste 2 (`putty-teste2-freezer.log`): após reiniciar, havia apenas o monitor do ELF único. A montagem antecipada ocorreu com `attempt=0`, antes do `EXEC` de `PPSA28180`; o jogo chegou a iniciar e usar save data. Não há `PRX_NOT_RESOLVED_FUNCTION`. Mais tarde o log mostra início da sequência de standby e depois `sceLncServiceKeepAlive` congelado e espera por `AppStateLock`. Não há `NOTE_EXIT` do jogo nem `unmount` no trecho capturado, porque o jogo foi suspenso, não encerrado. O log não prova que a montagem causou o bloqueio; também há `ajmBatchWait` preso em `SceShellUI` antes de o monitor BackPork iniciar. O usuário precisou forçar o desligamento.

Próxima investigação: confirmar em que momento visual ocorreu o congelamento e comparar, em boot limpo, a mesma sequência de jogo/standby com a base A53+kstuff sem BackPork. Não gerar outro build por mera alteração de temporização sem evidência para separar os caminhos de ShellUI, PPR e montagem.

## Esclarecimento do usuário

O usuário **não solicitou modo de repouso no teste 2**. Após carregar o ELF e abrir o jogo, a imagem congelou enquanto o áudio continuou normal; só depois ele forçou o desligamento. Portanto, a sequência `Timed out. Start standby sequence` é posterior ao congelamento percebido e não deve ser tratada como seu gatilho. O primeiro indício de bloqueio no log é `ajmBatchWait` em `SceShellUI` durante a etapa A53/PPR, antes de `[BP] native monitor started`. O jogo ainda executou e acessou save data após `[BP] mounted early`; o log não identifica o instante exato em que a imagem deixou de atualizar. A causa permanece indeterminada. Teste 1 também teve dois monitores sobrepostos e, apesar de o usuário relatar que o jogo funcionou, terminou com erros de limpeza `common/lib`.
