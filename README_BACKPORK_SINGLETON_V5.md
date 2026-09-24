# BackPork singleton v5 — experimental

O log de 2026-09-24 mostra duas cargas do ELF v4. Na primeira, A53/PPR foi instalado, o monitor iniciou, mas `PPSA28180` sofreu `PRX_NOT_RESOLVED_FUNCTION` antes da montagem. Na segunda, os dois monitores ficaram ativos: ambos montaram o mesmo `common/lib`, o jogo abriu, uma desmontagem retornou `errno=16` (`EBUSY`) e o ShellCore não conseguiu remover o sandbox. Assim, o segundo envio não valida a v4 como solução de uma carga.

Esta v5 mantém a base A53/PPR/rest mode e o código nativo BackPork em **um ELF, sem ELF interno nem fork**. Ela tenta montar `fakelib` já no `NOTE_CHILD`, antes da resolução de PRX, com `NOTE_EXEC` como tentativa de recuperação. A fila única e a tabela por PID seguem a abordagem do [BackPork original](https://github.com/BestPig/BackPork/blob/master/main.c). A montagem é liberada no `NOTE_EXIT`, sem remover diretórios do sandbox.

O monitor obtém um `flock` não bloqueante em `/data/kstuff-backpork-monitor.lock` e mantém o descritor aberto; um segundo processo integrado não inicia outro monitor. O arquivo de lock pode permanecer em `/data`, mas o bloqueio é liberado pelo sistema quando o processo termina. Esse mecanismo só evita duplicação entre cópias desta integração, não com um BackPork original carregado separadamente.

Build com PS5 SDK, `-Wall -Werror`: sucesso. SHA-256 do ELF: `5ccc9bb143fbe1fab43d23ae5c162923bedfd6776324444a6e7cba1cd5600dba`.

Teste no console em 2026-09-24 (`putty.log`): **funcionou com uma carga do ELF**. O A53/PPR terminou com resultado 0 em 3159 ms e o kstuff ficou pronto em 3401 ms. Um único monitor BackPork iniciou (`pid=95`). `PPSA28180` e `PPSA17221` tiveram `fakelib` montada antes do `EXEC` na primeira tentativa e desmontada ao fechar, com `rc=0 errno=0` para ambos. Não há `PRX_NOT_RESOLVED_FUNCTION`, erro fatal ou segunda inicialização do monitor nesse registro. Há `inherited mount missed` para `PPSA03530`; o log não demonstra que esse título precisava de `fakelib`.

O registro termina durante a suspensão do sistema. Portanto, esta sessão **não valida o retorno do repouso nem o reinício** com a v5. Também não prova estabilidade em sessões prolongadas. A v2 chegou a montar cedo e congelou a imagem em boot limpo, então mantenha os testes de repouso/reinício separados dos testes de abertura de jogos. Evite carregar outro BackPork ou reenviar a v5 na mesma sessão.

## Resultado após o repouso

O usuário relatou que, após voltar do repouso, o jogo não inicia com a v5. O `putty.log` disponível termina em `SYSTEM SUSPEND`, antes da retomada; por isso ainda não é possível atribuir a falha ao PPR ou ao monitor BackPork. O código PPR é idêntico ao da base `fix/ppr-restmode-v2`. Ele aguarda 20 amostras de estado WORKING e 12 amostras sem processo de jogo, a 250 ms por amostra, antes de verificar/reinstalar o PPR. Abrir o jogo imediatamente após acordar pode manter a verificação pendente. O monitor BackPork não tem rearmamento explícito do `kqueue` ao voltar do repouso; ele só recria a fila se `kevent` falhar ou o SysCore sair. Ambas são hipóteses a verificar no klog após a retomada, não causas confirmadas.
