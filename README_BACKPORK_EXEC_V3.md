# BackPork NOTE_EXEC v3: candidato experimental

Base A53/PPR e kstuff: `fix/ppr-restmode-v2`. BackPork derivado do [projeto original BestPig/BackPork](https://github.com/BestPig/BackPork), GPL-3.0, integrado como código C no mesmo ELF.

No build v2, BackPork montava em `NOTE_CHILD`, antes do `EXEC`; em boot limpo com PPSA28180, o usuário relatou vídeo congelado e áudio normal. No teste aparentemente bem-sucedido havia também outro BackPork ativo, de modo que dois monitores montaram o mesmo caminho. O controle com kstuff+A53/rest mode e BackPork original separados funciona, segundo o usuário.

Esta variante registra um watcher para o PID imediatamente em `NOTE_CHILD` e só monta ao receber `NOTE_EXEC`, reproduzindo o gatilho do BackPork original. Ela usa um worker por PID para não bloquear o monitor ao esperar a saída do jogo. A montagem acontece apenas quando `fakelib` e `common/lib` existem. Em `NOTE_EXIT`, desmonta sem esperar o sandbox desaparecer e sem remover diretórios do sistema. Não há fork nem ELF BackPork embutido. Somente kstuff exibe notificação gráfica.

Build SDK PS5: passou com `-Wall -Werror`. SHA-256 do ELF: `2255e83f71b8391abd6cdda677d8c0692b6ab1d62569e5c9d605b9fb620ac49c`.

**Não testado em console.** A compilação não demonstra que o watcher separado verá `NOTE_EXEC` em tempo suficiente, nem que o jogo ou a interface continuarão estáveis. Como a versão anterior congelou a imagem, este ELF deve ser tratado estritamente como teste. Para isolar o resultado, carregar uma única vez após boot limpo, sem outro BackPork ativo, e capturar o klog desde o envio até fechar o jogo. Os marcos esperados são `[BP] exec watch armed`, `[BP] exec observed`, `[BP] mounted after exec` antes de qualquer crash do jogo, e `[BP] unmount ... rc=0` após fechar. `exec not observed` ou `post-exec mount missed` indicam falha da abordagem. Não reutilizar a versão v2 EARLY para esse teste.
