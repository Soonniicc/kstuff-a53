# BackPork nativo v7 — rearmamento após repouso

O monitor BackPork recebe um sinal quando a rotina de energia detecta a transição de suspensão para `WORKING`. A fila `kqueue` é verificada a cada 250 ms. Se não houver montagem de jogo ativa, a v7 fecha a fila antiga, registra novamente `SceSysCore.elf` e emite no klog `resume: BackPork activated again`. A notificação gráfica `BackPork active again!` só é enviada após a primeira montagem `fakelib` bem-sucedida **antes do `EXEC`** na geração retomada. Se houver uma montagem ativa, adia o rearmamento até o jogo sair para não desmontar sua `common/lib` durante o uso. A notificação não aparece na carga inicial nem apenas por observar o estado `WORKING` ou registrar a fila; depende de o BackPork montar uma `fakelib` após o repouso.

Esta versão inclui a tentativa de montagem antecipada da v6. Nenhuma alteração foi feita na rotina de instalação A53/PPR. O último teste da v6 após repouso não avaliou BackPork: `PPSA28180` não chegou a criar processo; houve erros de I/O em `ssd0.system_ex` e `ssd0.system_data` e `SceShellUI` caiu antes de o PPR terminar. A v7 não corrige nem explica esses erros.

Teste esperado no log após retorno: `[BP] resume: rearm requested`, `[BP] resume: rebuilding SysCore watch`, `[BP] resume: BackPork monitor rearmed`, e, ao lançar jogo com `fakelib`, `[BP] mounted before exec` seguido de `[BP] resume: BackPork early mount confirmed`. A notificação confirma que houve montagem antecipada; não garante que o jogo continuará sem outros erros.

Build com PS5 SDK e `-Wall -Werror`: sucesso. ELF: `C:\Users\PC\Downloads\kstuff-a53-backpork-RESUME-v7-EXPERIMENTAL.elf`, SHA-256 `42ba90cfe1f5b7a9f3bf83d3ca025aab479b7c2e53b224e891b479774436fb98`. Ainda não validado no console. A notificação pode deixar de aparecer se nenhum jogo com `fakelib` for iniciado após o repouso ou se a montagem antecipada falhar; nesse caso o klog diferencia monitor rearmado de montagem não confirmada.

## Resultado no console após repouso

O `C:\Users\PC\Music\putty.log` iniciado em 2026-09-24 07:08:58 confirma a retomada do monitor na geração 2: `rearm requested`, `rebuilding SysCore watch` e `BackPork monitor rearmed syscore=53`. `[PPR] resume: patch verified` precedeu o lançamento dos jogos. `PPSA17221` montou `fakelib` antes do `EXEC` (pid 195, attempt 0), registrou `BackPork early mount confirmed` e desmontou com `rc=0 errno=0` ao fechar. `PPSA28180` também montou antes do `EXEC` (pid 207, attempt 0) e desmontou com `rc=0 errno=0`. Não foram encontrados `PRX_NOT_RESOLVED_FUNCTION`, sinal fatal nem os erros de I/O do teste anterior no registro analisado. O usuário informou que os jogos funcionaram. O klog confirma o caminho de montagem; a exibição da notificação gráfica ainda não foi confirmada pelo usuário.

Esse teste confirma rearmamento e montagem dos dois títulos nessa sessão. Ainda não estabelece estabilidade em todos os ciclos de repouso/reinício ou para todos os jogos.
