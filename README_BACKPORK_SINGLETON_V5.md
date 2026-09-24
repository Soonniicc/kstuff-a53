# BackPork singleton v5 — experimental

O log de 2026-09-24 mostra duas cargas do ELF v4. Na primeira, A53/PPR foi instalado, o monitor iniciou, mas `PPSA28180` sofreu `PRX_NOT_RESOLVED_FUNCTION` antes da montagem. Na segunda, os dois monitores ficaram ativos: ambos montaram o mesmo `common/lib`, o jogo abriu, uma desmontagem retornou `errno=16` (`EBUSY`) e o ShellCore não conseguiu remover o sandbox. Assim, o segundo envio não valida a v4 como solução de uma carga.

Esta v5 mantém a base A53/PPR/rest mode e o código nativo BackPork em **um ELF, sem ELF interno nem fork**. Ela tenta montar `fakelib` já no `NOTE_CHILD`, antes da resolução de PRX, com `NOTE_EXEC` como tentativa de recuperação. A fila única e a tabela por PID seguem a abordagem do [BackPork original](https://github.com/BestPig/BackPork/blob/master/main.c). A montagem é liberada no `NOTE_EXIT`, sem remover diretórios do sandbox.

O monitor obtém um `flock` não bloqueante em `/data/kstuff-backpork-monitor.lock` e mantém o descritor aberto; um segundo processo integrado não inicia outro monitor. O arquivo de lock pode permanecer em `/data`, mas o bloqueio é liberado pelo sistema quando o processo termina. Esse mecanismo só evita duplicação entre cópias desta integração, não com um BackPork original carregado separadamente.

Build com PS5 SDK, `-Wall -Werror`: sucesso. SHA-256 do ELF: `5ccc9bb143fbe1fab43d23ae5c162923bedfd6776324444a6e7cba1cd5600dba`.

**Não testado no console.** A v2 também montou cedo, mas em boot limpo o usuário relatou imagem congelada e áudio normal; por isso não há garantia de que esta v5 seja estável. Teste no máximo uma carga após boot limpo, sem outro BackPork. Capture o klog desde o envio até fechar o primeiro jogo. O sinal mínimo é `[BP] native singleton monitor started`, `[BP] mounted before exec` antes de qualquer `PRX_NOT_RESOLVED_FUNCTION`, e `[BP] unmount ... rc=0` após a saída. Se aparecer `monitor already active or lock unavailable`, `mounted inherited` só após crash ou congelamento, não repetir o envio. Para uso normal, permaneça na combinação A53/rest mode + BackPork original separado que funciona.
