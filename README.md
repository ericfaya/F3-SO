1r pas executem makefile en els 3 terminals(bowman,poole,discovery ),executem make clean i make per cada un.

2n pas Preferiblement fer primer make clean de bowman,despres de poole i despres de discovery,o a vegades abans discovery que poole(per tema de addres binding socket...)

3r pas Un cop hem fet make clean i make per cada un podem fer discovery discovery_config.dat poole poole_config.dat bowman bowman_config.dat

4t pas Escribim CONNECT a bowman,i ja podem descarregar cançons,hem deixat el usleep,sabem que nomes funcionen cançons sequencials i no en paral·lel.

# SO-FINAL
