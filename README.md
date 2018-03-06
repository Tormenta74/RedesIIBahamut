# Practica 1

## Servidor HTTP en C

### Diego Sáinz de Medrano y Fernando Villar

---

## Introducción

---

## Desarrollo técnico
Comenzamos el desarrollo de nuestro servidor HTTP con un proceso iterativo de construcción de servidores. La primera aproximación estaba basada en código procedente de la misma práctica de esta asignatura.

Comenzamos resolviendo los problemas de las librerías:
1. `libtcp`
Esta librería actúa como capa de contacto entre las funciones de bajo nivel de sockets, como `socket`, `recv`, `bind`, etc., para proporcionar una interfaz consistente con estas funciones.
2. `libconcurrent`
Con esta librería añadimos una capa más simple al uso de los diferentes items de la librería `pthread`, de la que usamos tanto los mutex como los hilos.
3. `libdaemon`
Esta miniatura es simplemente una caja para la función `daemonize` que utilizamos para poner los procesos servidores en segundo plano.

Empezamos desarrollando unas funciones "esqueleto" para servidores, donde configuramos la dirección IP y el puerto en el que estamos escuchando, atando un socket a dicho puerto, y lanzando el servidor en modo demonio (`src/server.c:server_setup`). La segunda función, fundamental para el desarrollo, es `src/server.c:server_accept_loop`.
En una primera instancia, esta función se basaba en el funcionamiento de `select` sobre un set de sockets abiertos, de forma que usábamos el tipo `fd_set` para almacenar los sockets a los que están conectados los clientes, y despachábamos hilos de atención a cada cliente ya conectado o usábamos la función `accept` para conectarles a un nuevo socket, dependiendo del número de socket. Sin embargo, esta aproximación era compleja incluso de leer, así que la descartamos por el siguiente esquema multihilo:
```
while(active):
	tcp_accept(&new_socket)	   // llamada bloqueante
	print(addr)    // mostramos la IP y el puerto del cliente
	launch(attention_routine, new_socket)    // lanzamos un hilo que atiende a dicho cliente
```
Donde `attention_routine` es un puntero a una función compatible con los hilos de `pthread` que se lanza como hilo.

A partir de aquí, podemos implementar el protocolo que queramos en la rutina, así que como primer prototipo implementamos un sencillo eco:
```
echo(socket):
	tcp_recv(socket, buffer)    // bloqueante
	tcp_send(socket, buffer)    // devolvemos el mismo mensaje
	tcp_close(socket)
```
Una vez probado este esquema, probamos con un servidor de ficheros:
```
file_contents = read(file_pointer, file_size)

[...]

file(socket):
	tcp_recv(socket, buffer)    // esperamos a que hable el cliente
	tcp_send(socket, file_contents)
	tcp_close(socket)
```



---

## Conclusiones técnicas

## Conclusiones personales
