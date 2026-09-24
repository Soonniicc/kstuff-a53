# BackPork NOTE_TRACK v4 — experimental

O controle do usuário é kstuff+A53/rest mode em boot limpo, seguido pelo BackPork original separado: os jogos que precisam de BackPork abrem. A v3 integrada observava `NOTE_EXEC` em uma nova fila por filho, mas recebia o evento somente após `PRX_NOT_RESOLVED_FUNCTION` nos dois jogos testados.

Nesta variante, o processo `kstuff.elf` mantém **uma fila kqueue** para `SceSysCore.elf` com `NOTE_TRACK`, `NOTE_EXEC` e `NOTE_EXIT`, como no [BackPork original](https://github.com/BestPig/BackPork/blob/master/main.c). `NOTE_TRACK` registra automaticamente o evento do filho na mesma fila, conforme o [manual FreeBSD de kqueue](https://man.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2). O código registra os PIDs filhos e monta somente ao receber `NOTE_EXEC` nessa fila. Ele não bloqueia esperando o jogo sair: ao receber `NOTE_EXIT`, desmonta o caminho que montou. Não remove o sandbox. Evita duas montagens internas para o mesmo `common/lib`.

A53/PPR/rest mode vem da base previamente testada. BackPork está compilado diretamente neste único ELF, sem segundo ELF embutido e sem `fork`. Só kstuff exibe notificação gráfica. O módulo BackPork é derivado do código de BestPig, GPL-3.0.

Build PS5 SDK no Docker: `-Wall -Werror`, sucesso. SHA-256: `ce41f00b62947021fd9a9d26dadd2817ae326988c7aaeb0c54b0d3babc754c86`.

**Ainda não testado no console.** O manual FreeBSD descreve o evento, mas não demonstra que o monitor no processo do kstuff será agendado a tempo no PS5. A v2 integrada congelou a imagem, e a v3 deixou os jogos falharem; este build continua experimental. Para um teste isolado, carregar somente este ELF uma vez após boot limpo, sem BackPork separado, e capturar o klog. O sinal mínimo é `[BP] mounted inherited` antes de qualquer `PRX_NOT_RESOLVED_FUNCTION`; após fechar o jogo, `[BP] unmount ... rc=0`. Se houver congelamento, não reenviar esta versão na mesma sessão.
