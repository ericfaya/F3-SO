1r pas executem makefile en els 3 terminals(bowman,poole,discovery ),executem make clean i make per cada un.

2n pas Preferiblement fer primer make clean de bowman despres make,seguidament lo mateix per poole(make clean i make) i despres de discovery,o a vegades abans discovery que poole(per tema de addres binding socket...)

3r pas Un cop hem fet make clean i make per cada un podem fer l'execucio directament amb valgrind per poder veure els errors de memoria:
valgrind --leak-check=full --show-leak-kinds=all discovery discovery_config.dat
valgrind --leak-check=full --show-leak-kinds=all poole poole_config.dat 
valgrind --leak-check=full --show-leak-kinds=all bowman bowman_config.dat
valgrind --tool=helgrind bowman bowman_config.dat
Cal destacar que els confis.dat estan organitzats perque el proces Bowman s'executi en el servidor matagalls,proces poole s'ha de executar en el servidor montserrat,i el proces discovery en el servidor de puigpedros.
Cal destacar tambe que els nostres ports anaven del 8105 al 8109 i del 8070 al 8074
Matagalls Server: BOWMAN
ens160 (External Interface):
IP Address: 172.16.205.3
ens192 (Internal Interface):
IP Address: 192.168.1.3

Montserrat Server: POOLE
ens160 (External Interface):
IP Address: 172.16.205.4
ens192 (Internal Interface):
IP Address: 192.168.1.4

Puigpedros Server:DISCOVERY
ens160 (External Interface):
IP Address: 172.16.205.17
ens192 (Internal Interface):
IP Address: 192.168.1.5

Erfaes888

4t pas Escribim CONNECT a bowman,i ja podem descarregar cançons,hem deixat el usleep per asegurar el funcionament en paralel de les descarregues.

# SO-FINAL
