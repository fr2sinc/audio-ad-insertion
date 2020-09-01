# Audio-ad-insertion


# Utilizzo


# Configurazione ed esecuzione

## 1) Requisiti
1. Installare Visual Studio Community 2017 15.9.25 (nell'installare selezionare solo kit c++) https://visualstudio.microsoft.com/it/vs/older-downloads/
2. Installare JUCE https://juce.com/get-juce/download
3. Estrarre lo zip di JUCE e spostare in una directory comoda (es. C:\)
4. Buildare l'host per debuggare i plugin: nella directory JUCE recarsi in `extras\AudioPluginHost\Builds\VisualStudio2017` aprire il progetto AudioPluginHost.sln ed eseguirlo in VisualStudio (selezionando come build Debug e x64). L'eseguibile dell'AudioPluginHost si troverà in `extras\AudioPluginHost\Builds\VisualStudio2017\x64\Debug\App`, copiarlo in `JUCE\` per comodità
5. Aprire il repo audio-ad-insertion, spostare la directory `audio-ad-insertion-data` nella cartella Documents del poprio user (per non avere dei path assoluti tutte le letture e scritture su disco saranno effettuate nella directory `Documents\audio-ad-insertion-data` .
6. Aprire il file FFTimpl.jucer tramite l'eseguibile di juce Projucer.exe
7. Nella schermata di projucer per avviare il progetto cliccare sull'icona di visual studio 2017, dovrebbe avviarsi tutto il progetto su vs.
8. Settare su Visual Studio l'AudioPluginHost: dal pannello degli strumenti Debug -> properties -> Configuration Properties -> Debugging -> inserire in Command il path dell'AudioPluginHost.exe che è stato buildato precedentemente.
9. Run del progetto audio-ad-insertion (configurazione Debug e x64), si avvierà l'audioPluginHost, da lì cliccare su Open ed aprire la configurazione esistente (dal repo audio-ad-insertion) ad-insertion-debug.filtergraph.
10. Apparirà una Gui in stile Max msp, se non è presente il plugin audio-ad-insertion trascinarlo da `Builds\VisualStudio2017\x64\Debug\VST3\FFTimpl.vst3` all'interno della GUI. Collegare MIDI Input -> Sine Wave Synth -> audio-ad-insertion -> Audio Output.
11. Inserire un breakpoint nel file FFT.h: funzione pushSampleIntoSongMatchFifo(); prima che avvenga generateHashes(); (per inserirlo selezionare la riga e cliccare f9)
12. Per triggerare il Tono e quindi l'audio Injection suonare la nota Fa (quarta ottava, (698 HZ)) fino a quando non viene iniettato un nuovo Jingle, questo verrà stoppato dopo 10 secondi, e ne verrà eseguito il riconoscimento da generateHashes(); all'interno di pushSampleIntoSongMatchFifo();
13. Dopodichè su disco saranno scritti tutti i picchi, (dei jingle analizzati in frequenza durante l'esecuzione del costruttore del Plugin, e dello stream audio live avviato con il riconoscimento del tono). 


# Author
