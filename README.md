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
