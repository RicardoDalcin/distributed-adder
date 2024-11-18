Comandos docker:

- Uma vez só:

  - `docker network create --subnet=192.168.1.0/24 udp_network`
  - `docker build -t cpp-build-container-alpine .`

- Para cada container (dentro do WSL2, senão o mount não funciona):
  - `docker run --rm -it --privileged --net udp_network --ip 192.168.1.100 -v /mnt/c/Users/usuario/caminho-projeto:/workspace cpp-build-container-alpine`
  - Varia o IP pra cada um (.100, .101, etc)

Todo

- Ver se enviar o tipo da requisição (DISCOVERY, REQUEST, REQUEST_ACK) impacta na performance
- Talvez separar as classes de serviços em Client e Server

Dúvidas
- Para timeout, usar receive não-bloqueante ou timeout do socket?
  - Timeout do socket tem problema que se receber outra mensagem, reinicia o timeout
- Logger deveria ser chamado no recebimento de uma nova request, ou escutar mudanças na tabela de clientes?
- Podemos usar estruturas de dados prontas, como map?
- Em caso de DUP, o servidor pode reenviar o ack?
- Devemos exibir alguma mensagem se chegar uma request fora de ordem?
- Na tabela de clientes, o last_sum 68 para 1.1.1.3 não deveria ser 111 (68 + 43)?