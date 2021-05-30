# CChat

Una implementazione di una chatroom in C.

per compilare il codice usare:
```html
make all
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

il client non richiede argomenti. Per far partire il client basta
```html
./client
```


Nella src c'è anche una cartella client_2. Essa contiene il progetto iniziale della gui per il client ormai abbandonato a causa di crash che avvolte avvengono
durante l'inizializzazione della gui e senza un apparente motivo.
```html
$ gcc -pthread `pkg-config --cflags gtk+-3.0` client.c -o client `pkg-config --libs gtk+-3.0`
```
per compilarlo, ma è funzionante solamente la parte della gui e non si connette effettivamente al server a causa di alcune feature mancanti (quali la scelta del nickname, l'invio formattato dei messaggi e la lettura di essi)
