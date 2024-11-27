# Sistemas Operacionais II

## Trabalho prático - Parte 1

### Integrantes

- Bernardo Beneduzi Borba, bbborba@inf.ufrgs.br - 00323911
- Ricardo Hermes Dalcin, rdalcin@inf.ufrgs.br - 00325735

### Descrição

Esse repositório contém o código referente à parte 1 do trabalho prático da disciplina de Sistemas Operacionais II.

Para compilar, basta executar o comando `make` na raiz do projeto.

Caso deseje compilar em modo debug, basta executar o comando `make DEBUG=1`.

Para executar o programa, basta rodar os seguintes comandos:

- Servidor:`./dist/server 4000`
- Cliente: `./dist/client 4000` ou `./dist/client 4000 < input.txt`

Etapa 2:

- Servers:
  - A: ./dist/server 4000 - Primário (se não receber nada de outros servidores quando enviar broadcast)
  - B: ./dist/server 4000 - Secundário
  - C: ./dist/server 4000 - Secundário
  - D: ./dist/server 4000 - Secundário
- Clients:
  1. Cliente manda broadcast: só servidor primário responde
  2. Client manda request com número
  3. Servidor primário recebe request, primeiro replica pros outros servers e depois pode dar ok pro client
  4. Servidor A vai ser puxado da tomada
  5. Roda algoritmo de eleição entre os três que sobraram
  6. Cliente vai dar timeout 3 vezes (servidor não responde mais), vai mandar broadcast pra descobrir novo servidor
- Pode fazer keep alive a cada 500ms e se o servidor primário não responder 3x, roda o algoritmo de eleição
- Só participa no algoritmo de eleição (valentão) quem tiver a tabela mais atualizada (relógios lógicos de Lamport)
  - Se todos tiverem atualizados, todos participam do algoritmo de eleição
- Não propaga só a soma, pode propagar a tabela 