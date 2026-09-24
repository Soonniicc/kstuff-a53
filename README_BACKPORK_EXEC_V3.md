# BackPork NOTE_EXEC v3: candidato experimental

Base A53/PPR e kstuff: `fix/ppr-restmode-v2`. BackPork derivado do [projeto original BestPig/BackPork](https://github.com/BestPig/BackPork), GPL-3.0, integrado como código C no mesmo ELF.

No build v2, BackPork montava em `NOTE_CHILD`, antes do `EXEC`; em boot limpo com PPSA28180, o usuário relatou vídeo congelado e áudio normal. No teste aparentemente bem-sucedido havia também outro BackPork ativo, de modo que dois monitores montaram o mesmo caminho. O controle com kstuff+A53/rest mode e BackPork original separados funciona, segundo o usuário.

Esta variante registra um watcher para o PID imediatamente em `NOTE_CHILD` e só monta ao receber `NOTE_EXEC`, reproduzindo o gatilho do BackPork original. Ela usa um worker por PID para não bloquear o monitor ao esperar a saída do jogo. A montagem acontece apenas quando `fakelib` e `common/lib` existem. Em `NOTE_EXIT`, desmonta sem esperar o sandbox desaparecer e sem remover diretórios do sistema. Não há fork nem ELF BackPork embutido. Somente kstuff exibe notificação gráfica.

Build SDK PS5: passou com `-Wall -Werror`. SHA-256 do ELF: `2255e83f71b8391abd6cdda677d8c0692b6ab1d62569e5c9d605b9fb620ac49c`.

**Não testado em console.** A compilação não demonstra que o watcher separado verá `NOTE_EXEC` em tempo suficiente, nem que o jogo ou a interface continuarão estáveis. Como a versão anterior congelou a imagem, este ELF deve ser tratado estritamente como teste. Para isolar o resultado, carregar uma única vez após boot limpo, sem outro BackPork ativo, e capturar o klog desde o envio até fechar o jogo. Os marcos esperados são `[BP] exec watch armed`, `[BP] exec observed`, `[BP] mounted after exec` antes de qualquer crash do jogo, e `[BP] unmount ... rc=0` após fechar. `exec not observed` ou `post-exec mount missed` indicam falha da abordagem. Não reutilizar a versão v2 EARLY para esse teste.

## Resultado no console

**Falhou nos dois jogos com BackPork; não reutilizar esta v3.** No log `C:/Users/PC/Music/putty.log` de 2026-09-23 20:51, `PPSA17221` registra `PRX_NOT_RESOLVED_FUNCTION` na linha 1914 e só depois `exec observed`/`mounted after exec` nas linhas 2325–2331. Para `PPSA28180`, o erro aparece na linha 2878 e a montagem depois nas linhas 3144–3147. Em ambos, a desmontagem foi `rc=0` após o crash; o defeito é a montagem tardia para resolver as PRX, não o cleanup. O controle com A53/rest mode e BackPork separado funciona segundo o usuário. A hipótese de que um worker aguardando `NOTE_EXEC` resolveria a corrida foi refutada.

Comparando com a v2 que montava em `NOTE_CHILD`, a v3 não mostra `ajmBatchWait` travado; ambas mostram coredumps MP4 durante a instalação A53/PPR. Portanto, o congelamento visual da v2 e a falha de PRX da v3 são sintomas diferentes, e o coredump MP4 por si só não identifica a causa do congelamento. Não produzir nova variante baseada apenas em atrasos arbitrários.
