# BackPork nativo v6 — tentativa antecipada após repouso

O `putty.log` capturado após o retorno do repouso mostra `[PPR] resume: patch verified`. `PPSA28180` montou a `fakelib` antes do `EXEC` (pid 95) e desmontou com `rc=0`. `PPSA17221`/Minecraft (pid 101) executou sem a linha `mounted before exec`, caiu com `PRX_NOT_RESOLVED_FUNCTION` e só depois recebeu `mounted inherited`. Nova tentativa do Minecraft (pid 131) repetiu a falha. `PPSA03530` chegou a executar; houve `inherited mount missed`, mas o log não mostra que esse jogo precisava de `fakelib`.

A v5 consultava `sceKernelGetAppInfo` apenas uma vez em `NOTE_CHILD`. A v6 faz até 20 consultas, com 1 ms entre elas, para aguardar um título `PPSA` ou `CUSA`; após identificá-lo, tenta encontrar e montar a `fakelib` até 50 vezes, também com pausa de 1 ms. Registra `child title not ready` ou `before-exec mount missed` quando não consegue montar antecipadamente. O caminho `NOTE_EXEC` continua apenas como diagnóstico/recuperação tardia. Não há alteração no A53/PPR, kstuff ou desmontagem.

Compilado com PS5 SDK (`-Wall -Werror`) no Docker. ELF: `C:\Users\PC\Downloads\kstuff-a53-backpork-CHILD-RETRY-v6-EXPERIMENTAL.elf`. SHA-256: `17443a65b710d79b9ae629fadea45dbd354a5bd8574114c277ca6948b52cd4b3`.

**Ainda não testado no console.** A causa exata da perda da janela de `NOTE_CHILD` ainda é uma hipótese: o log v5 não registrava falhas da consulta AppInfo nem da tentativa de montagem. Testar uma única carga em boot limpo, abrir Minecraft, fechar e então testar repouso/retorno. Capturar klog do teste, sobretudo linhas `[BP] child title not ready`, `[BP] before-exec mount missed`, `[BP] mounted before exec`, `PRX_NOT_RESOLVED_FUNCTION` e `[PPR] resume:`. Não carregar BackPork separado nem reenviar este ELF na mesma sessão.
