# BackPork v8 — correção do nome do sandbox

Após boot limpo com a v7, a primeira abertura de `PPSA28180` falhou. O klog registrou dezenas de `unionfs mount failed ... errno=2` com destino corrompido (`/mnt/sandbox/PPSA28180_000/P<.../common/lib`), seguido de `before-exec mount missed` e `PRX_NOT_RESOLVED_FUNCTION`. Mais tarde, `PPSA17221` e a segunda tentativa de `PPSA28180` montaram caminhos válidos antes do `EXEC` e desmontaram com `rc=0`. O PPR tinha instalado com resultado 0 antes do primeiro lançamento.

A causa no código é uso de memória após `closedir(dir)` em `find_random_folder`: `entry->d_name` pertence ao diretório, mas a v7 chamava `strdup(entry->d_name)` após fechá-lo. A v8 duplica o nome antes de `closedir`. Essa é a única mudança de comportamento em relação à v7; rearmamento após repouso e a notificação após montagem antecipada permanecem.

Build com PS5 SDK e `-Wall -Werror`: sucesso. ELF: `C:\Users\PC\Downloads\kstuff-a53-backpork-RESUME-v8-FIXED-EXPERIMENTAL.elf`, SHA-256 `3713495c9c4fe4ff819507e0b9c3490c7694adbe97dbaef8dc363339497da0f5`.

Ainda não testado no console. Um segundo log após repouso com a v7 está pendente; ele não altera o diagnóstico do caminho corrompido antes do repouso. Ao testar a v8 em boot limpo, o ponto decisivo é a primeira abertura do jogo que precisa de `fakelib`: destino legível, `mounted before exec`, jogo funcional e `unmount ... rc=0` ao fechar. Depois, repetir o ciclo de repouso.
