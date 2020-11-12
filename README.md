Il Software utilizza una directory di supporto:
**audio-ad-insertion-data**, che per comodità viene ricercata
automaticamente dal plugin all’interno della directory documenti della
macchina su cui esso girerà. I sorgenti sono compilabili previa
installazione di JUCE[1] ed uno tra gli IDE supportati all’esportazione
(Xcode, Visual Studio, Code::Blocks, Clion) con relativi compilatori. La
compilazione è stata testata con JUCE 6.0.1, Visual Studio Community
2017 ed il kit "Desktop development with c++".

[1] <https://juce.com/get-juce/download>

### Analisi offline

Per la generazione del database degli hash, è necessario:

1.  creare una directory **audio-ad-insertion-dataaudioDatabase**
    contenente tutti i jingle che saranno presenti all’interno dello
    stream radiofonico da analizzare. Possono essere utilizzati file
    audio dei formati più comuni loseless e lossy.

2.  aprire il progetto projucer **FingerprintLoader**, esportarlo su un
    IDE di preferenza tra quelli disponibili, ed avviare il processo di
    compilazione.

3.  avviare l’eseguibile **FingerprintLoader** ottenuto dal processo di
    compilazione, e dall’interfaccia, selezionare il sampleRate di default
    che sarà utilizzato all’interno della DAW / host che ospiterà il
    plugin.

4.  una volta completata l’operazione di caricamento degli hash,
    all’interno di audio-ad-insertion-data sarà creata una directory
    dataStructures contenente i file json delle strutture dati, che il
    plugin leggerà in fase di avvio.

    
### Esecuzione del Plugin all’interno di una DAW

Per attivare il processo di riconoscimento e sostituzione degli spot
pubblicitari:

1.  aprire il progetto projucer audio-ad-insertion, dal quale
    selezionare il formato del plugin che si desidera ottenere (VST3,
    AU, AUv3, RTAS, AAX, Standalone, Unity, VST (Legacy)), tenendo
    presente che per il formato VST(Legacy) è necessario prima fornirsi
    dell’SDK VST (versione testata:
    vstsdk367\_03\_03\_2017\_build\_352); ed esportare il tutto su un
    IDE come nel caso precedente.

2.  avviare la compilazione, e spostare il binario ottenuto nella
    directory utilizzata dalla DAW per la ricerca dei plugin 64bit.

3.  creare una traccia all’interno della DAW su cui istanziare il
    plugin, ed infine collegare lo stream radiofonico in ingresso alla
    traccia tramite routing. La DAW su cui è stato eseguito il plugin è
    Ableton Live, con il formato VST(legacy).

Il plugin in fase di injection pubblicitaria, leggerà i file audio
presenti all’interno della directory
**audio-ad-insertion-dataaudioInjection**.

### Demo

Per una veloce demo in ambiente Windows, nel software è già fornito un
eseguibile FingerprintLoader.exe ed il VST(legacy)
audio-ad-insertion.dll, dei jingle di test in
**audio-ad-insertion-dataaudioDatabase** con i relativi hash a 44100 Hz
in **audio-ad-insertion-datadataStructures**, e delle pubblicità
personalizzate in **audio-ad-insertion-data audioInjection**. Per
comodità in **audio-ad-insertion-datastream** sono state inserite anche
delle sezioni stream di esempio, contenenti i jingle da riconoscere, che
possono essere caricate all’interno della DAW, simulando uno streaming
in ingresso, su cui applicare il plugin per la sostituzione in tempo
reale. Basterà quindi spostare la cartella **audio-ad-insertion-data**
in documenti, importare il plugin nella DAW, settare in essa lo stesso
samplerate usato per la generazione degli hash (44100 HZ), caricare uno
stream di esempio, ed avviare la riproduzione.
