# CChat

Una implementazione di una chatroom in C.

per compilare il server e il client da terminale:
```html
make all
```
se si vuole compilare anche il client con la gui <strong>(in questo caso verrà chiesto di scaricare il package gtk)</strong> :
```html
make all-gui
```

Per rimuovere i compilati
```html
make clear
```
mentre per rimuovere la libreria scaricata
```html
make clear-dep
```


Il compilato verrà messo nella cartella bin (anche essa creata dal makefile).
I file di log verranno messi nella stessa cartella del programma client o server.

<hr class="rounded">

<h2>Server</h2>

Per far partire il server spostarsi nella cartella bin/server e far partire il programma
```html
./server [port] [mode] [millis_check]
```

<strong>port</strong> indica la porta su cui il server dovrà ascoltare. la porta di default è la 5000.

<strong>mode</strong> indica la modalità con cui far partire il server. Il server mette a  disposizione due mode:
<ul>
  <li>
    1. RECEIVE_MODE : il timestamp che conta è quello di ricezione del server. (default)
  </li>
  <li>
    2. TIMESTAMP_MODE : il timestamp che conta è quello di invio da parte del client.
  </li>
</ul>

<strong>millis_check</strong> indica quanti millisecondi il reader thread deve andare in sleep una volota inviati i messaggi della coda. 
In TIMESTAMP_MODE questo argomento influenza anche quanto un messaggio deve aspettare in coda prima di essere mandato.

<hr class="rounded">

<h2>Client</h2>

Il source code del client da terminale si trova nella cartella src/client mentre quello del client con la gui
si trova in src/client_2/client

il client non richiede argomenti. Per far partire il client basta
```html
./client
```
o
```html
./gui_client
```
nel caso del client con l'interfaccia grafica.

