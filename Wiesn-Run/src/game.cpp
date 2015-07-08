#include "game.h"
#include "movingobject.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <cmath>

#include <fstream>
#include <thread>

#include "player.h"
#include "gameobject.h"
#include "enemy.h"
#include "shoot.h"
#include "menu.h"





/**
 * @brief Verglecht zwei GameObjects, bezüglich der X-Position
 * @param 1.Objekt
 * @param 2.Objekt
 * @return true, wenn 1.Objekt weiter links als 2.Objekt
 * @author Simon
 */
struct compareGameObjects {
    bool operator()(GameObject *objA, GameObject *objB) {
        return objA->getPosX() < objB->getPosX();
    }
};


/**
 * @brief Vergleich zweier Scores
 * Der Vergleich findet über die Summe der Punkte in den einzelnen Kategorien statt.
 * Der Operator im struct ist mit größer (>) programmiert, da die Liste absteigend sortiert werden soll.
 * @author Simon
 */
struct compareScores {
    bool operator()(scoreStruct scoreA, scoreStruct scoreB) {
        return (scoreA.totalPoints > scoreB.totalPoints);
    }
};


/**
 * @brief Konstruktor
 * Initialisiert den appPointer
 * @param argc
 * @param argv
 * @author Rupert
 */
Game::Game(int argc, char *argv[]) : QObject() {
    /// Initialisiert den appPointer mit der QApplication
    appPointer = new QApplication(argc,argv);

}


Game::~Game() {

}


/**
 * @brief wird regelmäßig aufgerufen
 * event muss drinstehen, damit der Timer die Funktion aufruft
 * @param event
 * @author Rupert
 */
void Game::timerEvent(QTimerEvent *event)
{
    Q_UNUSED (event)        // Warnung unterdrücken
    step();
    ///@todo return von step...
}


/**
 * @brief Die Startfunktion, erstellt Fenster und Menüs, wird von main() aufgerufen
 * Grafik und Inputs (Flo,Felix):
 * Erstelle QApplication app mit QGraphicsView Widget window (Eventfilter installiert) und Zeiger input auf Input Objekt.
 * Um Funktionen der Tastatur Eingabe entwickeln zu können ist ein Qt Widget Fenster nötig.
 * Auf dem Widget wird ein Eventfilter installiert welcher kontinuierlich Tastureingaben mitloggt.
 * Die Eingaben werden in dem Objekt der Input Klasse gespeichert und können über getKeyactions() abgerufen werden.
 *
 * Logik (Rupert):
 * Außerdem wird ein Timer gestartet, der in jedem Intervall timerEvent(...) aufruft, wo dann step() aufgerufen wird.
 * Das ist dann unsere Game-Loop. Der Timer funktioniert auch bei 5ms Intervall noch genau.
 * Menüs (Rupert):
 * Alle Menüs werden angelegt
 *
 * gameState wird auf gameMenuStart gesetzt, dh das Spiel startet im Startmenü
 *
 * @return Rückgabewert von app.exec()
 * @author Rupert
 */
int Game::start() {
    qDebug("Game::start()");

    // Zufall initialisieren
    srand(time(NULL));



    // Fundamentale stepSize setzen
    stepIntervall = 1000/frameRate;
    //stepIntervall = 100; zum testen

    // Menüs erstellen
    menuInit();

    /// Erstelle Audiocontrol Objekt zum Einlesen der Audiodatein und speichern der Ausgabeparameter
    audioOutput = new AudioControl;
    /// Erstelle einen neuen Thread portaudiothread.
    /// Initialisiere dort PortAudio und beginne eine Audioausgabe zu erzeugen.
    std::thread portaudiothread(&AudioControl::playInitialize, audioOutput);

    // QGraphicsScene der Level erstellen
    levelScene = new QGraphicsScene;

    // QGraphicsView Widget (Anzeigefenster) erstellen und einstellen
    window = new QGraphicsView();
    window->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    window->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    window->setFixedSize(1024,768);
    window->setWindowTitle(QApplication::translate("Game Widget", "Wiesn - Run"));
    window->setEnabled(false);
    window->show();
    qDebug("initialize window");

    /// Installiere Event Filter zum Loggen der Keyboard Eingabe
    window->installEventFilter(keyInput);

    ///@todo hier wird das Startmenü übersprungen
    //startNewGame("level1_old.txt",1);
    //setState(gameMenuStart);
    setState(gameMenuStart);


    // Timer installieren
    qDebug("Starte Timer mit 500msec-Intervall");
    Game::startTimer(stepIntervall);

    return appPointer->exec();
}


/**
 * @brief Startet neues Spiel
 * lädt Leveldatei
 * füllt worldobjects
 */
void Game::startNewGame(QString levelFileName, int levelNum) {


    // alles alte leeren
   levelScene->clear();
    worldObjects.clear();

    //Levelscene einstellen
    levelScene->setSceneRect(0,0,100000,768);
    window->setScene(levelScene);


    //level nummer speichern
    gameStats.actLevel = levelNum;
    // Level festlegen, der geladen werden soll
    QString fileSpecifier = ":/levelFiles/levelFiles/";
    fileSpecifier.append(levelFileName);
    loadLevelFile(fileSpecifier);

    //Backgroundgrafiken initialisieren
    backgrounds = std::vector<QGraphicsPixmapItem>(4);

    backgrounds[0].setPixmap(QPixmap(":/images/images/bg_lev1_1.png"));
    backgrounds[1].setPixmap(QPixmap(":/images/images/bg_lev1_2.png"));
    backgrounds[2].setPixmap(QPixmap(":/images/images/bg_lev1_3.png"));
    backgrounds[3].setPixmap(QPixmap(":/images/images/bg_lev1_4.png"));

    //Backgroundgrafiken positionieren
    backgrounds[1].setPos(2560,0);
    backgrounds[3].setPos(2560,0);

    //Backgroundgrafiken der Scene hinzufügen
    for(int i=0; i<4; i++) {
        levelScene->addItem(&backgrounds[i]);
    }
    // Spieler hinzufügen
    worldObjects.push_back(playerObjPointer);
    //Grafik - Spieler der Scene hinzufügen und window auf ihn zentrieren
    levelScene->addItem(playerObjPointer);
    window->centerOn(playerObjPointer->getPosX(), 384);
    // Szenen-Breite setzen
    sceneWidth = 1024;
    // audioIDs initialisieren
    audioIDs = 20;

    playerStats = std::vector<QGraphicsTextItem>(4);

    playerStats[0].setPlainText(QString("Gesundheit: " + QString::number(playerObjPointer->getHealth())));
    playerStats[0].setPos(playerObjPointer->getPosX()-95, 30);
    playerStats[0].setDefaultTextColor(Qt::red);
    playerStats[0].setFont(QFont("Times",25));

    playerStats[1].setParentItem(&playerStats[0]);
    playerStats[1].setPlainText(QString("Alkoholpegel: " + QString::number(playerObjPointer->getAlcoholLevel())));
    playerStats[1].setPos(0, 50);
    playerStats[1].setDefaultTextColor(Qt::black);
    playerStats[1].setFont(QFont("Times",25));

    playerStats[2].setParentItem(&playerStats[0]);
    playerStats[2].setPlainText(QString("Score: " + QString::number(playerScore.totalPoints)));
    playerStats[2].setPos(670, 0);
    playerStats[2].setDefaultTextColor(Qt::yellow);
    playerStats[2].setFont(QFont("Times",35));

    playerStats[3].setParentItem(&playerStats[0]);
    playerStats[3].setPlainText(QString("Munition: " + QString::number(playerObjPointer->getAmmunatiuon())));
    playerStats[3].setPos(0, 100);
    playerStats[3].setDefaultTextColor(Qt::darkGreen);
    playerStats[3].setFont(QFont("Times",25));

    levelScene->addItem(&playerStats[0]);

    // Score initialisieren
    playerScore.alcoholPoints = 0;
    playerScore.distanceCovered = 0;
    playerScore.enemiesKilled = 0;
    playerScore.name = "Sepp";
    playerScore.totalPoints = 0;

    stepCount = 0;

    // Zeiger auf Objekte aus levelInitial in worldObjects verlegen
    /// @todo Theoretisch brauchen wir levelIntial nicht mehr, die Frage ist, ob die Grafik da mitmacht.
//    while (!(levelInitial.empty())) {
//        GameObject *currentObject = *levelInitial.begin();
//        worldObjects.push_back(currentObject);
//        levelInitial.pop_front();
//        //Grafik
//        levelScene->addItem(currentObject);
//    }
}


/**
 * @brief Game::endGame
 * Diese Funktion löscht nicht mehr nötige Variablen und Objekte wenn vom Spiel in das Statistikmenü gewechselt wird.
 * es werden auch die Statistik und Highscoremenüs aktualisiert
 * @ author: Felix, Johann
 */
void Game::endGame() {
    // Namen Menü
    const string bavarianNames[208] = {"Ade", "Ali", "Loi", "Alosius", "Anderl", "Andi", "Ande", "Bäda", "Bare", "Bartl", "Baste", "Bastia", "Bastl", "Horstl", "Bene", " Benny", "Bep", "Beppa", "Beppe", "Bepperl", "Berne", "Hardl", "Ber", "Bert", "Berti", "Bini", "Blase", "Chris", "Christl", "Christ", "Christà", "Dam", "Dama", "Douma", "Doilfa", "Doilferl", "Don", "Dona", "Doner", "Dan", "Dannerl", "Donisl", "Ed", "Ederl", "Ferdl", "Fest", "Fesl", "Fidl", "Fips", "Flore", "Fonse", " Fone", "Franze", "Franzl", "Fre", "Fredl", "Fridl", "Fritz", "Girgl", "Gog", "Gog", " Gode", "Gottl", "Gore", "Grisch", " Grisse", "Guste", " Gustl", " Guste", "Hans", " Hanse", " Hans", " Hanse", " Hann", " Haan", " Hannas", "Hart", " Harte", "Hausl", "Heini", "Hias", "Irgei", "Jacher", " Jaherl", "Jack", "Joc", "Jock", "Jockei", "Kar", " Karle", "Kaschb", " Koschba", "Kone/", " Kon", "Korbe", " Korw", " Kurwe", "Laure", "Len", " Lenze", " Lenze", "Le", " Poldi", "Loi", " Lois", " Loise/", " Loise", " Lui", " Luis", " Luisele", "Lug", " Lugge", " Wickerl", "Lugge", "Maiche", "Mane", "Marine", " Nuss", " Mareale", "Mart", " Masch", " Madd", " Maschde", " Mart", " Mort", " Mäa", " Märt", "Mat", " Mat", " Matte", " Matze", "Ma", " Max", " Maxl", "Michl", " Miche", " Müche", " Mih", " Müh", " Misch", " Michei", "Muck", "Naze", " Naa", " Nadsl", "Nic", " Nicke", " Nickl", "Polde", " Poidl", "Quire", " Quirl", "Reine", "Remme", "Rude", " Rudi", " Rul", "Rups", "Schoasch", " Schorschl", " Schor", " Schosc", " Schoo", " Schos", " Schurli", "Sepp", " Seppe", " Sepp", " Sepper", " Seppei", "Siege", " Siggi", "Simmal", "Stachus", "Steffl", " Steffe", " Stefferl", "Stoff", " Stoff", " Stofferl", "Ton", " Tona", " Tonerl", "Vale", "Veit", " Veit", "Vere", " Verl", "Vie", " Vinz", "Voitl", "Was", " Waste", " Wastl", "Wic", " Wiggl", " Wigge", " Wigger", " Wack", "Woife", " Woifer", " Wolle", "Xandi", "Xar", " Xavi", " Xaver", " Xide", "Zenz"};
    //std::string bavarianNames[10] = {"Ade", "Loisl", "Anderl", "Bäda", "Bertl", "Wickerl", "Beppi", "Hias", "Xaver", "Miche"};

    int nameMax = 208;  //10;
    int nameIndex = rand() % nameMax;

    menuName->clear();
    menuName->addEntry(bavarianNames[nameIndex],menuId_NonClickable);
    menuName->addEntry("weida",menuNameId_Next,true,gameMenuStatisitcs);
    menuName->displayInit();

    playerScore.name = bavarianNames[nameIndex];
    setState(gameMenuName);

    /// @todo Aufräumarbeiten
    // Highscore aktualisieren
    updateHighScore("write");

    //Listen leeren
    audioevents.clear();
    audioStorage.clear();

    stepCount = 0;

    playerObjPointer = 0;
    while (!(worldObjects.empty())) {
        GameObject *handleObject = worldObjects.front();
        worldObjects.pop_front();
        delete handleObject;
    }

    /*while (!(levelInitial.empty())) {
        GameObject *handleObject = levelInitial.front();
        levelInitial.pop_front();
        delete handleObject;
    }*/

    while (!(levelSpawn.empty())) {
        GameObject *handleObject = levelSpawn.front();
        levelSpawn.pop_front();
        delete handleObject;
    }
}


/**
 * @brief Game::exitGame
 *        Diese Funktion löscht nicht mehr nötige Variablen und Objekte wenn das Spiel komplett beendet wird.
 * @author: Felix
 */
void Game::exitGame() {

    /// Beende Audio Ausgabe und lösche Audiobezogene Variablen

    /// rufe Desktrutor Objekt audioOutput auf
    /// Stoppe PortAudio Audioausgabe, Beende Portaudio Stream, Beende PortAudio und lösche Objekt audioOutput
    delete audioOutput;
}


/**
 * @brief Game-Loop
 * Diese Funktion wird von timerEvent() aufgerufen und ist für den kompletten Ablauf des Spiels verantwortlich.
 * grober Ablauf:
 * LOOP:
 *  - Timer starten
 *  - Neue Objekte zur Welt hinzufügen
 *  - alte Objekte löschen
 *  - Input auslesesn
 *  - Bewegungen berechnen
 *  - Kollisionskontrolle
 *  - Bewegungen korrigieren
 *  - Events behandeln (Treffer..)
 *  - Grafik rendern und ausgeben
 *  - Audio ausgeben
 *  - verbleibende Zeit im Slot berechnen (Timer auslesen)
 *  - entsprechend warten
 * goto LOOP
 * @return 0 bei fehlerfreiem Beenden
 * @author Rupert
 */
int Game::step() {
    using namespace std::chrono;

    /// Tasten abfragen
    Input::Keyaction lastKey = keyInput->getLastKeyaction();
    Menu *aktStepMenu = aktMenu;

    /// Zeit seit dem letzten Aufruf ausrechnen und ausgeben

    //high_resolution_clock::time_point akt = letzterAufruf;
    //letzterAufruf = high_resolution_clock::now();
    //qDebug("Game::step() | Vergangene Zeit seit letztem step(): %d ms", static_cast<int>(duration_cast<milliseconds>(letzterAufruf-akt).count()));

    // falls Menü aktiv, Inputs verarbeiten, Grafik:

    if(aktStepMenu!=NULL) { // aktMenu ist nur NULL, wenn das Spiel gerade läuft


        //AudioEvent falls level begonnen wird
        audioCooldownstruct backgroundLevel;
        aktStepMenu->displayUpdate();

        //MenüScene wird vom Anzeigewidget aufgerufen
        window->setScene(aktStepMenu->menuScene);

        // Audio
        if(aktStepMenu->getType() == Menu::menuType::highscore) {
            // Audioevent für die Statistik-Menüs
            audioStruct scoreAudio = {2, background_highscore, audioDistance.background_highscore};
            audioevents.push_back(scoreAudio);
        } else {    // Menu::menuType::normal
            //Audioevent für die Spiel-Menüs
            audioStruct menuAudio = {1, background_menu, audioDistance.background_menu};
            audioevents.push_back(menuAudio);
        }

        // Up || Down?
        if(lastKey == Input::Keyaction::Up) {
            aktStepMenu->changeSelection(Menu::menuSelectionChange::up);
        }
        if(lastKey == Input::Keyaction::Down) {
            aktStepMenu->changeSelection(Menu::menuSelectionChange::down);
        }

        // Enter auswerten
        if(lastKey == Input::Keyaction::Enter) {
            if(aktStepMenu->getSelection()->menuOnEnter) {  // Alle Einträge, auf die ein weiteres Menü folgt
                setState(aktStepMenu->getSelection()->stateOnClick);
            }
            // Zusatzaufgaben, die in den Menüs ausgeführt werden
            switch(aktStepMenu->getSelection()->id) {
                case menuStartId_EndGame:
                    exitGame();
                    exit(0);
                case menuLevelId_Demo:
                    startNewGame("level_test.txt",1);
                    setState(gameIsRunning);
                    break;
                case menuLevelId_Level1:
                    startNewGame("level1.txt",1);
                    setState(gameIsRunning);
                    break;
                case menuLevelId_Level2:
                    startNewGame("level2.txt",2);
                    setState(gameIsRunning);
                    break;
                case menuLevelId_Level3:
                    startNewGame("level3.txt",3);
                    setState(gameIsRunning);
                    break;
                case menuBreakId_Resume:
                    window->setScene(levelScene);
                    setState(gameIsRunning);
                    break;
                case menuBreakId_EndGame:
                    setState(gameMenuStart);
                    break;
                case menuBreakId_EarlyEnd:
                    endGame();
                    break;
                case menuNameId_Next:
                    displayStatistics();
                    break;
            }
        }
        // Namenseingabe
        if(aktStepMenu==menuName) {
            Input::Keyletter key = keyInput->getLastKeyletter();
            if(key!=0) { // Nur wenn Taste gedrückt wurde, Menü neuschreiben
                menuName->clear();
                std::string name = playerScore.name;
                char letter = static_cast<char>(key);

                if(letter=='\b' && name.length()>0) {  // Backspace
                    name.pop_back();    // Zeichen löschen
                } else {
                    name.push_back(letter);
                }
                playerScore.name = name;

                menuName->addEntry(name,menuId_NonClickable);
                menuName->addEntry("Weida",menuNameId_Next,true,gameMenuStatisitcs);
                menuName->displayInit();
            }
        }

    } else {    // gameIsRunning

        // Escape: Pause
        if(lastKey == Input::Keyaction::Exit) {
            setState(gameMenuBreak);
        }
        /// @todo Levelende hier abfragen

        worldObjects.sort(compareGameObjects());
        qDebug("---Nächster Zeitschritt---");

        appendWorldObjects(playerObjPointer);
        reduceWorldObjects(playerObjPointer);

        // Audio
        if (stepCount == 0) {
            audioCooldownstruct newAudio;
            newAudio.audioEvent = {5, background_startgame, audioDistance.background_startgame};
            newAudio.cooldown = audioCooldown.background_startgame;
            audioStorage.push_back(newAudio);
        }
        updateAudio();

        evaluateInput();
        worldObjects.sort(compareGameObjects());
        calculateMovement();
        worldObjects.sort(compareGameObjects());
        detectCollision(&worldObjects);
        handleCollisions();

        updateScore();

        renderGraphics(&worldObjects, playerObjPointer);



        // Mockup: add audioStruct player_jump to audioevents list
        //audioStruct player_jump{1, audioType::background_level1, 0.6};
        // Mockup: add audioStruct powerup_beer to audioevents list
        //audioevents.push_back(player_jump);
        //audioStruct scene_beer{2, audioType::background_level1, 0.6};
        //audioevents.push_back(scene_beer);
        // send filled audioevents list to AudioControl Object, which updates current Output Sounds


        // Level zu Ende?
        if (playerObjPointer->getPosX() - playerScale >= levelLength) {
            endGame();
            setState(gameMenuName);
            // Erfolgreich Schriftzug einfügen

            // Sound für erfolgreichen abschluss spielen
            audioCooldownstruct newAudio;
            newAudio.audioEvent = {4, background_levelfinished, audioDistance.background_levelfinished};
            newAudio.cooldown = audioCooldown.background_levelfinished;
            chrono::high_resolution_clock::time_point previous = chrono::high_resolution_clock::now();
            while ( newAudio.cooldown > chrono::duration<int>(0)) {
                audioevents.push_back(newAudio.audioEvent);
                audioOutput->update(&audioevents);
                audioevents.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                chrono::high_resolution_clock::time_point actual = chrono::high_resolution_clock::now();
                newAudio.cooldown = newAudio.cooldown - chrono::duration_cast<std::chrono::milliseconds> (actual - previous);
                previous = actual;
            }
        }

        // Spieler tot?
        if (gameStats.gameOver) {
            endGame();
            setState(gameMenuName);
            // GameOver schriftzug einfügen

            //Audio event wenn der Spieler stirbt
            audioCooldownstruct newAudio;
            newAudio.audioEvent = {4, status_dead, audioDistance.status_dead};
            newAudio.cooldown = audioCooldown.status_dead;
            chrono::high_resolution_clock::time_point previous = chrono::high_resolution_clock::now();
            while ( newAudio.cooldown > chrono::duration<int>(0)) {
                audioevents.push_back(newAudio.audioEvent);
                audioOutput->update(&audioevents);
                audioevents.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                chrono::high_resolution_clock::time_point actual = chrono::high_resolution_clock::now();
                newAudio.cooldown = newAudio.cooldown - chrono::duration_cast<std::chrono::milliseconds> (actual - previous);
                previous = actual;
            }
        }

        stepCount++;
    }

    // Audio ausgabe außerhalb des If-Statements damit auch in den Menüs Musik ausgegben wird
    audioOutput->update(&audioevents);
    /// delete List audioStruct elements in list and fill it in the next step again
    audioevents.clear();

    return 0;
}


/**
 * @brief Game::appendWorldObjects
 * @param playerPointer
 * Diese Funktion fügt der Spielwelt dynamisch Gegner hinzu. In jedem Zeitschritt wird die sortierte Liste
 * levelSpawn vom Anfang her durchlaufen. Ist die Distanz des Spielers zum Gegner kleiner als die Distanz levelSpawn,
 * so wird das Objekt den worldObjects hinzugefügt und aus levelSpawn gelöscht. Die for-Schleife läuft solange, bis
 * das erste Mal ein Objekt weiter als levelSpawn vom Spieler entfernt ist. Dann wird abgebrochen, da alle folgenden
 * Objekte auf Grund der Sortierung noch weiter entfernt sein werden.
 * @author Simon
 */
void Game::appendWorldObjects(Player *playerPointer) {
    //Grafik - der Spieler wird vor der aktualisierung der Worldobjects entfernt
    levelScene->removeItem(playerObjPointer);

    while (!(levelSpawn.empty())) {
        GameObject *currentObj = *levelSpawn.begin();
        if ( (currentObj->getPosX() - playerPointer->getPosX()) < spawnDistance ) {
            worldObjects.push_back(currentObj);
            levelSpawn.pop_front();
            //Grafik - Gegner der Scene hinzufügen
            levelScene->addItem(currentObj);
        } else {
            break;
        }
    }
    //Grafik - der Spieler wird wieder hinzugefügt damit er immer das Grafikelement an vorderster Stelle ist
    levelScene->addItem(playerPointer);
}


/**
 * @brief Game::reduceWorldObjects
 * @param playerPointer
 * Die Funktion reduceWorldObjects löscht die Zeiger auf die GameObjects aus dem Spiel, von denen der Spieler bereits
 * weiter rechts als die spawnDistance entfernt ist.
 * Entfernt Bierkrüge, die im letzten Durchlauf mit Gegenständen kollidiert sind
 * @todo Objekte löschen anstatt nur die Zeiger aus der Liste entfernen
 * @author Simon, Johann
 */
void Game::reduceWorldObjects(Player *playerPointer) {

    //Entferne die Bierkrüge die an Wände oder Gegner, etc. gestoßen sind.
    objectsToDelete.sort(compareGameObjects());
    while (!(objectsToDelete.empty())) {
        GameObject *currentObject= *objectsToDelete.begin();
        //use worldObjects.erase(position)
        std::list<GameObject*>::iterator it = worldObjects.begin();
        while((*it != currentObject) && (it != worldObjects.end())) {
            it++;
        }
        if (*it == currentObject) {
            worldObjects.erase(it);
            objectsToDelete.pop_front();
            //Grafik - Bierkrüge löschen
            levelScene->removeItem(currentObject);
            //Speicher freigeben
            delete currentObject;
        }
    }

    //Entferne die Objekte die sich links vom Bildschirm befinden
    while (!(worldObjects.empty())) {
        GameObject *currentObj = *worldObjects.begin();
        if ((playerPointer->getPosX() - currentObj->getPosX()) > deleteDistance) {
            worldObjects.pop_front();
            //Grafik - Objekte aus der Scene löschen
            levelScene->removeItem(currentObj);
            //Speicher freigeben
            delete currentObj;
        } else {
            break;
        }
    }

    //Entferne die Objekte, die sich rechts aus dem Bildschirm entfernen
    while (!(worldObjects.empty())) {
        GameObject *currentObj = worldObjects.back();
        if ((currentObj->getPosX() - playerObjPointer->getPosX()) > (spawnDistance + 1)) {
            worldObjects.pop_back();
            //Grafik - Objekte aus der Scene löschen
            levelScene->removeItem(currentObj);
            //Speicher freigeben
            delete currentObj;
        } else {
            break;
        }
    }

}


/**
 * @brief Checkt welche Tasten für die Spielkontrolle gedrückt sind
 * mögliche Tasten:
 *  - Pfeil rechts zum laufen
 *  - Pfeil hoch zum springen
 *  - Leertaste zum schießen
 *  - ESC für Menü
 * @author Rupert
 */
void Game::evaluateInput() {
    // Pfeil rechts?
    if(keyInput->getKeyactions().contains(Input::Keyaction::Right)) {
        playerObjPointer->setSpeedX(playerSpeed);
        if (!(playerObjPointer->inJump())) {
            // Audioevent erzeugen
            audioStruct playerAudio = {19, player_walk, 0};
            audioevents.push_back(playerAudio);
        }
    } else {
        playerObjPointer->setSpeedX(0);
    }

    // Pfeil oben?
    if(keyInput->getKeyactions().contains(Input::Keyaction::Up)) {
        if (!(playerObjPointer->inJump())) {
            playerObjPointer->startJump();
            // Audioevent erzeugen
            audioCooldownstruct playerjump;
            playerjump.audioEvent= {audioIDs, player_jump, audioDistance.player_jump};
            playerjump.cooldown = audioCooldown.player_jump;
            audioIDs = audioIDs + 1;
            audioStorage.push_back(playerjump);
        }
    }

    // Leertaste?
    if(keyInput->getKeyactions().contains(Input::Keyaction::Shoot)) {
        if ((playerObjPointer->getAmmunatiuon() > 0) && (playerObjPointer->getFireCooldown() < 1)) {
            Shoot *playerFire = new Shoot(playerObjPointer->getPosX()+playerObjPointer->getLength()/2,playerObjPointer->getPosY(),1,player);
            playerObjPointer->decreaseAmmunation();
            playerObjPointer->setFireCooldown();
            worldObjects.push_back(playerFire);
            levelScene->addItem(playerFire);
        }
    }
}


/**
 * @brief Geht die worldObjects durch und aktualisiert bei jedem die Position,
 *      Gegner bei denen der DeathCooldown abgelaufen ist, werden zum loeschen vorgemerkt,
 *      Gegner bei denen der FireCooldownabgelaufen ist feuern.
 * wird momentan auch über Debug ausgegeben
 * @author Rupert, Johann
 */
void Game::calculateMovement() {
    using namespace std;               // für std::list

    /// für qDebug (Rupert)
    std::string objecttypes[] = {"Player ", "Tourist", "Securit", "Obstacl", "Plane  ", "Shot   ", "PowerUp", "Boss   "};
    qDebug("Object\tSize\tPosX\tPosY\tSpeed");
    int speedX=0,speedY=0;

    list<GameObject*>::iterator it;     // Iterator erstellen
    /// Schleife startet beim ersten Element und geht bis zum letzen Element durch
    for(it = worldObjects.begin(); it != worldObjects.end(); ++it) {
        speedX = 0; speedY = 0;         // für Debug
        GameObject *aktObject = *it;
        MovingObject *aktMovingObject = dynamic_cast<MovingObject*> (aktObject);    // Versuche GameObject in Moving Object umzuwandeln
        if(aktMovingObject != 0) {
            aktMovingObject->update();          // Wenn der cast klappt, rufe update() auf.
            //falls es sich um einen Gegner handelt feuern
            if ((aktMovingObject->getType() == enemy_security) || (aktMovingObject->getType() == enemy_tourist)){
                Enemy *aktEnemy = dynamic_cast<Enemy*> (aktMovingObject);
                if (aktEnemy->getDeathCooldown() == 0) {
                    objectsToDelete.push_back(aktEnemy);
                } else if (aktEnemy->getFireCooldown() == 0) {
                    Shoot *enemyFire;
                    int direction;
                    /* Bewegungrichtung für den Krug ermitteln
                     * bei bewegendem Gegner in Richtung der Bewegung
                     * bei stillstehenden in Richtung des Spielers
                     */
                    if (aktEnemy->getSpeedX() == 0) {
                        direction = playerObjPointer->getPosX() - aktEnemy->getPosX();
                    } else {
                        direction = aktEnemy->getSpeedX();
                    }
                    if (direction < 0) {
                        direction = -1;
                    } else {
                        direction = 1;
                    }
                    enemyFire = new Shoot(aktEnemy->getPosX(), aktEnemy->getPosY(), direction, aktEnemy->getType());
                    worldObjects.push_back(enemyFire);
                    levelScene->addItem(enemyFire);
                    enemyFire = 0;
                }
                if (((aktEnemy->getPosX() - playerObjPointer->getPosX()) == sceneWidth) && aktEnemy->getSpeedX() > 0) {
                    aktEnemy->setSpeedX(-aktEnemy->getSpeedX());
                }
                aktEnemy = 0;
            }
            speedX = aktMovingObject->getSpeedX();
            speedY = aktMovingObject->getSpeedY();
        }

        aktMovingObject = 0;
        // Anzeige: Object: HöhexBreite, xPos,YPos, (vx,vy)
        qDebug("%s\t%dx%d\t%4d\t%4d\t(%3d,%3d)",objecttypes[static_cast<int>(aktObject->getType())].c_str(),aktObject->getLength(),aktObject->getHeight(), aktObject->getPosX(),aktObject->getPosY(),speedX,speedY);

    }

}


/**
 * @brief Game::detectCollision
 * Diese Funktion berechnet ob Kollisionen zwischen benachbarten Objekten auftreten und falls ja, aus welcher
 * Richtung diese stattfinden.
 * Da die Liste worldObjects in jedem Zeitschritt sortiert wird, müssen die Kollisionen nur für die nächsten Nachbarn
 * berechnet werden. Allerdings können durch ungünstige Lage auch Objekte kollidieren, die nicht direkt nebeneinander
 * in der Liste liegen. Dafür werden fünf Nachbarn links und rechts jedes MovingObjects geprüft, falls vorhanden.
 * @author Simon
 */
void Game::detectCollision(std::list<GameObject*> *objectsToCalculate) {

    for (std::list<GameObject*>::iterator it=objectsToCalculate->begin(); it != objectsToCalculate->end(); ++it) {
        // Wähle den aktuellen Zeiger aus und versuche einen Downcast auf MovingObject*
        MovingObject *affectedObject = dynamic_cast<MovingObject*>(*it);

        // Prüfe, ob der Downcast erfolgreich was
        if (affectedObject != 0) {
            // Downcast war erfolgreich, affectedObject ist vom Typ MovingObject

            // Lege Liste möglicher Kollisionen an
            std::list<GameObject*> possibleCollisions;

            // Füge die vorhergehenden fünf Elemente hinzu, falls sie vorhanden sind
            int counter = 0;
            while ((counter < 5) && (std::prev(it, counter) != objectsToCalculate->begin())) {
                possibleCollisions.push_back(*std::prev(it, counter + 1));
                ++counter;
            }

            // Füge die nächsten fünf Elemente hinzu, falls sie vorhanden sind
            counter = 0;
            while ((counter < 5) && (std::next(it, counter) != objectsToCalculate->end())) {
                possibleCollisions.push_back(*std::next(it, counter + 1));
                ++counter;
            }

            // Durchlaufe die Schleife möglicher Kollisionen, bis sie leer ist
            while (!(possibleCollisions.empty())) {
                // Setze causingObject auf das erste Element in der Liste und lösche den ersten Listeneintrag
                GameObject *causingObject = *possibleCollisions.begin();
                possibleCollisions.pop_front();

                // Überprüfe, ob das affectedObject links bzw. oberhalb des causingObjects ist und speichere die Ergebnis in bool Variablen
                // Auch wenn der Basispunkt jedes Objekts unten Mitte ist: Für die Relation wird überprüft, ob die Mitte von aObj links / oberhalb der Mitte von cObj ist.
                bool affectedLeftFromCausing = (affectedObject->getPosX() - causingObject->getPosX()) < 0;
                bool affectedAboveCausing = (affectedObject->getPosY() + (affectedObject->getHeight() / 2) - (causingObject->getPosY() + causingObject->getHeight() / 2)) > 0;

                int overlayX;
                int overlayY;

                // Ist aObj links von cObj, so errechnet sich die Überlappung in X-Richtung als: Rechte Kante aObj - Linke Kante cObj, sonst Rechte Kante cObj - Linke Kante aObj
                if (affectedLeftFromCausing) {
                    overlayX = (affectedObject->getPosX() + (affectedObject->getLength() / 2)) - (causingObject->getPosX() - (causingObject->getLength() / 2));
                } else {
                    overlayX = (causingObject->getPosX() + (causingObject->getLength() / 2)) - (affectedObject->getPosX() - (affectedObject->getLength() / 2));
                }

                // Ist aObj über cObj, so errechnet sich die Überlappung in Y-Richtung als: Obere Kante cObj - Untere Kante aObj, sonst Obere Kante aObj - Untere Kante cObj
                if (affectedAboveCausing) {
                    overlayY = (causingObject->getPosY() + causingObject->getHeight()) - (affectedObject->getPosY());
                } else {
                    overlayY = (affectedObject->getPosY() + affectedObject->getHeight()) - (causingObject->getPosY());
                }

                // Ist die Y-Überlappung größer als die X-Überlappung und beide Überlappungen positiv, so findet eine horizontale Kollision statt.
                if ((overlayY > overlayX) && (overlayX > 0)) {
                    // Prüfe, ob die horizontale Kollision von Links oder von Rechts stattfindet
                    if (affectedLeftFromCausing) {
                        collisionStruct collision = {affectedObject, causingObject, fromLeft};
                        collisionsToHandle.push_back(collision);
                    } else {
                        collisionStruct collision = {affectedObject, causingObject, fromRight};
                        collisionsToHandle.push_back(collision);
                    }
                }

                // Ist die X-Überlappung größer als die Y-Überlappung und beide Überlappungen positiv, so findet eine vertikale Kollision statt.
                if ((overlayX > overlayY) && (overlayY > 0)) {
                    // Prüfe, ob die vertikale Kollision von Oben oder von Unten stattfindet
                    if (affectedAboveCausing) {
                        collisionStruct collision = {affectedObject, causingObject, fromAbove};
                        collisionsToHandle.push_back(collision);
                    } else {
                        collisionStruct collision = {affectedObject, causingObject, fromBelow};
                        collisionsToHandle.push_back(collision);
                    }
                }

            } // Ende von while !empty
        } // Ende der if-Abfrage bzgl. des erfolgreichen Downcasts
    } // Ende der for-Schleife über die objectsToCalculate
} // function

/**
 * @brief Kollisionen in der Liste collisionsToHandle werden der Reihe nach aus Sicht des affectedObjects bearbeitet.
 * In einer Schleife wird das jeweils erst CollisionEvent bearbeitet. Dabei werden nur an dem Objekt affectedObject Änderungen vorgenommen.
 * Mögliche Objekte: Spieler(player), Gegner(enemy), Bierkrug(shot)
 * mögliche Kollision mit Spieler(player), Hindernis(obstacle), Gegner(enemy), Bierkrug(shot), Power-Up(powerUp)
 *
 * @author Johann (15.6.15)
 */
void Game::handleCollisions() {

    collisionStruct handleEvent;
    int overlap;
    Enemy *handleEnemy;
    Shoot *handleShoot;
    int obstacleCollisionCounter = 0;

    //Liste mit den Events abarbeiten
    while (!collisionsToHandle.empty()) {
        handleEvent = collisionsToHandle.front();
        collisionsToHandle.pop_front();

        switch (handleEvent.affectedObject->getType()) {

        case player: {
            /*Zusammenstöße des Spielers
             *  mit Hindernis, Gegner, Schüssen, PowerUps
             *
             * default: Spieler
             */
            switch (handleEvent.causingObject->getType()) {
            case plane:
            case obstacle: {
                /* Zusammenstoß mit Hindernis
                 * Bewegungsabbruch, Positionskorrektur
                 *      4 Möglichkeiten: von oben, unten, links, rechts
                 */
                switch (handleEvent.direction) {
                case fromLeft: {
                    if (!((handleEvent.causingObject->getType() == plane) && (playerObjPointer->getPosY() > handleEvent.causingObject->getPosY()))){
                        //Bewegung  in X-Richtungstoppen
                        playerObjPointer->setSpeedX(0);
                        //Überlappung berechnen und Spieler nach links versetzen
                        overlap = (playerObjPointer->getPosX() + (playerObjPointer->getLength() / 2)) - (handleEvent.causingObject->getPosX() - (handleEvent.causingObject->getLength() / 2));
                        playerObjPointer->setPosX(playerObjPointer->getPosX() - overlap);
                    }
                    break;
                }
                case fromRight: {
//                    //Bewegung in X-Richtung stoppen
//                    playerObjPointer->setSpeedX(0);
//                    //Überlappung berechnen und Spieler nach rechts versetzen
//                    overlap = (handleEvent.causingObject->getPosX() + (handleEvent.causingObject->getLength() / 2)) - (playerObjPointer->getPosX() - (playerObjPointer->getLength() / 2));
//                    playerObjPointer->setPosX(handleEvent.causingObject->getPosX() + handleEvent.causingObject->getLength());
                    break;
                }
                case fromAbove: {
                    /* Überlappung berechnen und Spieler nach obern versetzen
                     *      Sprungzustand zurücksetzten
                     */
                    overlap = (handleEvent.causingObject->getPosY() + handleEvent.causingObject->getHeight()) - playerObjPointer->getPosY();
                    playerObjPointer->setPosY(playerObjPointer->getPosY() + overlap);
                    playerObjPointer->resetJumpState();
                    break;
                }
                case fromBelow: {
                    //Wegen Zusammenstoß wird ein Fall initiiert
                    playerObjPointer->abortJump();
                    //Überlappung berechnen und Spieler nach unten versetzen
                    overlap = (playerObjPointer->getPosY() + playerObjPointer->getHeight()) - handleEvent.causingObject->getPosY();
                    playerObjPointer->setPosY(playerObjPointer->getPosY() - overlap);
                    //AudioAusgabe bei zusammenstoß von unten
                    obstacleCollisionCounter = obstacleCollisionCounter + 1;
                    if (obstacleCollisionCounter == 1){
                        //Audioevent
                        audioCooldownstruct newAudio;
                        newAudio.audioEvent = {audioIDs, scene_collision_obstacle, audioDistance.scene_collision_obstacle};
                        audioIDs = audioIDs + 1;
                        newAudio.cooldown = audioCooldown.scene_collision_obstacle;
                        audioStorage.push_back(newAudio);
                    }
                    break;
                }
                }
                break;
            }
            case enemy_security:
            case enemy_tourist: {
                /* Zusammenstoß mit Gegener
                 *      Spieler läuft in Gegner oder Gegner fällt auf Spieler (sonst siehe affectedObject==enemy)
                 *      Dem Spieler wird Schaden zugefügt und er erhält einen kurzen immunitätsbonus, falls nicht schon vorhanden
                 */

                if (!(handleEvent.direction == fromAbove)) {
                    //Überprüfen ob dem Spieler Schaden zugefügt werden kann
                    if (!(playerObjPointer->getImmunityCooldown() > 0)) {
                        handleEnemy = dynamic_cast<Enemy*>(handleEvent.causingObject);
                        if (!(handleEnemy->getDeath())) {
                            //Stirbt der Spieler durch den zugefügten Schaden -> GameOver
                            gameStats.gameOver = playerObjPointer->receiveDamage(handleEnemy->getInflictedDamage());
                            //Audioevent
                            audioCooldownstruct newAudio;
                            newAudio.audioEvent = {audioIDs, scene_collision_player, audioDistance.scene_collision_player};
                            audioIDs = audioIDs + 1;
                            newAudio.cooldown = audioCooldown.scene_collision_player;
                            audioStorage.push_back(newAudio);
                        }
                        //Referenz löschen
                        handleEnemy = 0;
                    }
                }
                break;
            }
            case BOSS: {
                /* Zusammenstoß mit Endgegner
                 */
                handleEnemy = dynamic_cast<Enemy*> (handleEvent.causingObject);
                if (!(playerObjPointer->getImmunityCooldown() > 0)) {
                    gameStats.gameOver = playerObjPointer->receiveDamage(handleEnemy->getInflictedDamage());
                    //Audioevent
                    audioCooldownstruct newAudio;
                    newAudio.audioEvent = {audioIDs, scene_collision_player, audioDistance.scene_collision_player};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.scene_collision_player;
                    audioStorage.push_back(newAudio);

                }
                //Referenz löschen
                handleEnemy = 0;
                break;
            }
            case shot: {
                // Spieler kriegt Schaden, Bierkrug zum löschen vormerken, treffen mit eigenem Krug nicht möglich
                handleShoot = dynamic_cast<Shoot*>(handleEvent.causingObject);
                if (!(playerObjPointer->getImmunityCooldown())) {
                    gameStats.gameOver = playerObjPointer->receiveDamage(handleShoot->getInflictedDamage());
                    //Bierkrug zum löschen vormerken
                    objectsToDelete.push_back(handleShoot);

                    //Audioevent Krug zerbricht
                    audioCooldownstruct newAudio;
                    newAudio.audioEvent = {audioIDs, scene_collision_flyingbeer, audioDistance.scene_collision_flyingbeer};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.scene_collision_flyingbeer;
                    audioStorage.push_back(newAudio);
                    //Audioevent Schaden Spieler
                    newAudio.audioEvent = {audioIDs, scene_collision_player, audioDistance.scene_collision_player};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.scene_collision_player;
                    audioStorage.push_back(newAudio);
                    //Referenz löschen
                    handleShoot = 0;
                }
                break;
            }
            case powerUp: {
                /* Zusammenstöße mit PowerUps
                 *      Der Spieler erhält zusatzfähigkeiten
                 */
                PowerUp *handlePowerUp = dynamic_cast<PowerUp*> (handleEvent.causingObject);
                playerObjPointer->increaseHealth(handlePowerUp->getHealthBonus());
                playerObjPointer->increaseAmmunation(handlePowerUp->getAmmunationBonus());
                playerObjPointer->setImmunityCooldown(handlePowerUp->getImmunityCooldownBonus());
                playerObjPointer->increaseAlcoholLevel(handlePowerUp->getAlcoholLevelBonus());
                //AudioEvent
                if (handlePowerUp->getPowerUPType() == food) {
                    audioCooldownstruct newAudio;
                    newAudio.audioEvent = {audioIDs, powerup_food, audioDistance.powerup_food};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.powerup_food;
                    audioStorage.push_back(newAudio);
                } else {
                    audioCooldownstruct newAudio;
                    newAudio.audioEvent = {audioIDs, powerup_beer, audioDistance.powerup_beer};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.powerup_beer;
                    audioStorage.push_back(newAudio);
                }
                //PowerUp zum loeschen vormerken
                objectsToDelete.push_back(handlePowerUp);
                handlePowerUp = 0;
                break;
            }
            default: {
                /* Zusammenstoß mit Spieler
                 *      nicht möglich
                 */
            }
            }
            break;
        } // end (case player)

        case enemy_security:
        case enemy_tourist: {
            /*Zusammenstöße des Gegners
             *  mit Spieler, Hindernis, Schüssen
             *
             * default: Gegner, PowerUps
             *      werden durchlaufen ohne Effekt
             */

            handleEnemy = dynamic_cast<Enemy*>(handleEvent.affectedObject);

            switch (handleEvent.causingObject->getType()) {
            case player: {
                /* Zusammenstoß mit Spieler
                 *      Spieler springt auf Gegner (sonst siehe affectedObject==player)
                 * Der Gegner wird getötet
                 */
                if (handleEvent.direction == fromBelow) {
                    if (handleEnemy->receiveDamage(playerObjPointer->getInflictedDamage())) {
                        playerObjPointer->increaseEnemiesKilled();
                        //Audioausgabe
                        audioCooldownstruct newAudio;
                        newAudio.audioEvent = {audioIDs, scene_collision_enemy, audioDistance.scene_collision_enemy};
                        audioIDs = audioIDs + 1;
                        newAudio.cooldown = audioCooldown.scene_collision_enemy;
                        audioStorage.push_back(newAudio);

                    }
                }
                break;
            }
            case plane:
            case obstacle: {
                /* Zusammenstoß mit Hindernis
                 *  Bewegungsabbruch, Positionskorrektur, neue Bewegungsrichtung
                 */
                if (handleEvent.direction == fromAbove) {
                    //Fall wird beendet, X-Bewegung uneingeschränkt
                    overlap = (handleEvent.causingObject->getPosY() + handleEvent.causingObject->getHeight()) - handleEnemy->getPosY();
                    handleEnemy->setPosY(handleEnemy->getPosY() + overlap);
                } else {
                    // Bewegungsabbruch, neue Bewegungsrichtung
                    handleEnemy->setSpeedX(-handleEnemy->getSpeedX());
                    //PositionsKorrektur
                    if (handleEvent.direction == fromLeft) {
                        //Gegner kommt von links
                        overlap = (handleEnemy->getPosX() + handleEnemy->getLength()) - handleEvent.causingObject->getPosX();
                        handleEnemy->setPosX(handleEnemy->getPosX() - overlap);
                    } else {
                        //Gegner kommt von rechts
                        overlap = (handleEvent.causingObject->getPosX() + handleEvent.causingObject->getLength()) - handleEnemy->getPosX();
                        handleEnemy->setPosX(handleEnemy->getPosX() + overlap);
                    }
                }
                break;
            }
            case shot: {
                /* Zusammenstoß mit Bierkrug
                 *  Fügt Schaden zu, falls von Spieler geworfen,
                 *  Bierkrug zum löschen vormerken
                 */
                handleShoot = dynamic_cast<Shoot*>(handleEvent.causingObject);
                if ((handleShoot->getOrigin() == player) && (!(handleEnemy->getDeath()))) {
                    //Schaden zufügen
                    handleEnemy->receiveDamage(handleShoot->getInflictedDamage());
                    playerObjPointer->increaseEnemiesKilled();
                    //Bierkrug zum löschen vormerken
                    objectsToDelete.push_back(handleShoot);

                    //Audioevent Krug zerbricht
                    audioCooldownstruct newAudio;
                    newAudio.audioEvent = {audioIDs, scene_collision_flyingbeer, audioDistance.scene_collision_flyingbeer};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.scene_collision_flyingbeer;
                    audioStorage.push_back(newAudio);
                    //Audioausgabe Schaden an Gegner
                    newAudio.audioEvent = {audioIDs, scene_collision_enemy, audioDistance.scene_collision_enemy};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.scene_collision_enemy;
                    audioStorage.push_back(newAudio);
                }
                handleShoot = 0;
                break;
            }
            default: {
                /*Zusammenstoß mit Gegner, PowerUp, BOSS
                 *      kein Effekt
                 */
            }
            }
            handleEnemy = 0;
            break;
        }//end (case enemy)

        case BOSS: {
            /*  Zusammenstöße des Endgegners.
             * Nur interesant, wenn er mit shot zusammenstößt alles andere keine Wirkung
             */
            if (handleEvent.causingObject->getType() == shot) {
                handleEnemy = dynamic_cast<Enemy*> (handleEvent.affectedObject);
                handleShoot = dynamic_cast<Shoot*> (handleEvent.causingObject);
                if (handleShoot->getOrigin() == player) {
                    //Schaden zufügen
                    if (handleEnemy->receiveDamage(handleShoot->getInflictedDamage())) {
                        playerObjPointer->increaseEnemiesKilled();
                    }
                    //Bierkrug zum löschen vormerken
                    objectsToDelete.push_back(handleShoot);

                    //Audioevent
                    audioCooldownstruct newAudio;
                    newAudio.audioEvent = {audioIDs, scene_collision_flyingbeer, audioDistance.scene_collision_flyingbeer};
                    audioIDs = audioIDs + 1;
                    newAudio.cooldown = audioCooldown.scene_collision_flyingbeer;
                    audioStorage.push_back(newAudio);
                }
                handleShoot = 0;
                handleEnemy = 0;
            }
            break;
        } // end (case BOSS)

        case shot: {
            /*Zusammenstöße des Bierkrugs
             * mit Hindernis
             *
             * default: Spieler, Gegner, PowerUps
             *      wird jeweils in der Situation Spieler/Gegner bearbeitet, bei PowerUps keinen effekt
             * Bierkrug löschen, bei Kollision mit Hindernis
             */
            if ((handleEvent.causingObject->getType() == obstacle) || (handleEvent.causingObject->getType() == plane)){
                objectsToDelete.push_back(dynamic_cast<Shoot*>(handleEvent.affectedObject));

                //Audioevent
                audioCooldownstruct newAudio;
                newAudio.audioEvent = {audioIDs, scene_collision_flyingbeer, audioDistance.scene_collision_flyingbeer};
                audioIDs = audioIDs + 1;
                newAudio.cooldown = audioCooldown.scene_collision_flyingbeer;
                audioStorage.push_back(newAudio);
            }
            break;
        }//end (case shot)

        default: {
            /*Zusammenstöße von Hindernis und PowerUps
             *      NICHT MÖGLICH!!!   da keine MovingObjects
             *
             * default nur um Fehlermeldungen zu vermeiden
             */
        }
        }
    }
}

/**
 * @brief durchläuft die Liste audioStorage, zählt die Cooldowns runter
 *  Die Soundevents die noch laufen, werden an die Liste SoundEvents übergeben. Die fertigen werden gelöscht.
 *
 * @author Johann
 */
void Game::updateAudio() {

    //Hintergrundmusik
    audioType levelBackground [3] {background_level1, background_level2, background_level3};
    audioStruct newAudio = {(10-gameStats.actLevel), levelBackground[gameStats.actLevel - 1], 0.5};
    audioevents.push_back(newAudio);

    //Warntöne Leben/Alcoholpegel
    switch (playerObjPointer->getHealth()) {
    case 1: {
        newAudio = {11, status_lifecritical, audioDistance.status_lifecritical};
        audioevents.push_back(newAudio);
        break;
    }
    case 2: {
        newAudio = {12, status_life , audioDistance.status_life};
        audioevents.push_back(newAudio);
        break;
    }
    }
    if (playerObjPointer->getAlcoholLevel() >maxAlcohol) {
        newAudio = {13, status_alcohol, audioDistance.status_alcohol};
        audioevents.push_back(newAudio);
    }

    // Alle Objekte durchlaufen und für die sich bewegenden die entsprechenden Sounds ausgeben
    for (std::list<GameObject*>::iterator it=worldObjects.begin(); it != worldObjects.end(); ++it) {
        GameObject *handleObject =  (*it);
        switch (handleObject->getType()) {
        case enemy_tourist: {
            if (handleObject->getAudioID() == 0) {
                handleObject->setAudioID(audioIDs);
                audioIDs = audioIDs + 1;
            }
            float distance = static_cast<float> ((std::abs(playerObjPointer->getPosX() - handleObject->getPosX()))) / static_cast<float> (sceneWidth);
            newAudio = {handleObject->getAudioID(), scene_enemy_tourist, distance};
            audioevents.push_back(newAudio);
            break;
        }
        case enemy_security: {
            if (handleObject->getAudioID() == 0) {
                handleObject->setAudioID(audioIDs);
                audioIDs = audioIDs + 1;
            }
            float distance = static_cast<float> ((std::abs(playerObjPointer->getPosX() - handleObject->getPosX()))) / static_cast<float> (sceneWidth);
            newAudio = {handleObject->getAudioID(), scene_enemy_security, distance};
            audioevents.push_back(newAudio);
            break;
        }
        case BOSS: {
            if (handleObject->getAudioID() == 0) {
                handleObject->setAudioID(audioIDs);
                audioIDs = audioIDs + 1;
            }
            float distance = static_cast<float> ((std::abs(playerObjPointer->getPosX() - handleObject->getPosX()))) / static_cast<float> (sceneWidth);
            newAudio = {handleObject->getAudioID(), scene_enemy_boss, distance};
            audioevents.push_back(newAudio);
            break;

        }
        case shot: {
            if (handleObject->getAudioID() == 0) {
                handleObject->setAudioID(audioIDs);
                audioIDs = audioIDs + 1;
            }
            float distance = static_cast<float> ((std::abs(playerObjPointer->getPosX() - handleObject->getPosX()))) / static_cast<float> (sceneWidth);
            newAudio = {handleObject->getAudioID(), scene_flyingbeer, distance};
            audioevents.push_back(newAudio);
            break;
        }
        default:{
            /* nothing to do
             * Im Fall von obstacle, plane, player, powerUp soll kein ton ausgegeben werden
             */
        }
        }
    }
    // Zeit seit dem letzten aufruf messen
    chrono::high_resolution_clock::time_point lastStep = thisStep;
    thisStep = chrono::high_resolution_clock::now();
    chrono::duration<int, milli> difference = chrono::duration_cast<std::chrono::milliseconds> (thisStep - lastStep);

    // Cooldown audios weiterzählen und bei ablauf löschen
    for (std::list<audioCooldownstruct>::iterator it = audioStorage.begin(); it != audioStorage.end(); it++) {
        if (it->cooldown > chrono::duration<int>(0)) {
            it->cooldown = it->cooldown - difference;
            audioevents.push_back(it->audioEvent);
        } else {
            it = audioStorage.erase(it);
        }
    }
}

/**
 * @brief Game::renderGraphics
 * Positionssaktualisierungen der Grafiken aller Beewglichen Objekte
 * @param objectList
 */
void Game::renderGraphics(std::list<GameObject*> *objectList, Player *playerPointer) {

    //Bewegunsparralaxe Positionsaktualisierung
    (backgrounds[0]).setPos(((backgrounds[0]).x()) + ((playerPointer->getPosX() - (playerScale/2) - (playerPointer->x())) /2), 0);
    (backgrounds[1]).setPos(((backgrounds[1]).x()) + ((playerPointer->getPosX() - (playerScale/2) - (playerPointer->x())) /2), 0);

    //Leben,Pegel,Munition,Highscore bleiben auf die View zentriert
    playerStats[0].setPos( playerStats[0].x() + (playerPointer->getPosX() - (playerScale/2) - playerPointer->x()), playerStats[0].y());

    //Leben,Highscore,Pegel,Munition wird aktualisiert
    playerStats[0].setPlainText(QString("Gesundheit: " + QString::number(playerPointer->getHealth())));
    playerStats[1].setPlainText(QString("Alkoholpegel: " + QString::number(playerPointer->getAlcoholLevel())));
    playerStats[2].setPlainText(QString("Score: " + QString::number(playerScore.totalPoints)));
    playerStats[3].setPlainText(QString("Munition: " + QString::number(playerPointer->getAmmunatiuon())));

    //Wenn der Spieler aus einerm Hintergrundbild "rausläuft" wird die Position nachvorne verschoben
    for(int i = 0; i<=3; i++) {
        if(playerPointer->getPosX() - playerOffset >= static_cast<int>(backgrounds[i].x()+ 2560)) {
               backgrounds[i].setPos(backgrounds[i].x() + 5120, 0);
        }
    }

    //Positionsaktualisierungen aller Movingobjects
    for (std::list<GameObject*>::iterator it = objectList->begin(); it != objectList->end(); ++it) {
        MovingObject *aktMovingObject = dynamic_cast<MovingObject*> (*it);
        if(aktMovingObject != 0) {

            //if(dynamic_cast<Shoot*> (aktMovingObject) == 0 ) {
            if( (aktMovingObject->getType() != shot && aktMovingObject->getType() != player)
                 || (aktMovingObject->getType() == player && playerPointer->inJump() == false) ) {
                //im letzten Frame vorwärst gelaufen?
                if(aktMovingObject->getSpeedX() > 0 ) {
                    if(aktMovingObject->getFramesDirection() < 0) {
                        aktMovingObject->setFramesDirection(0);
                        aktMovingObject->flipHorizontal();
                    }
                    if(aktMovingObject->getFramesDirection()%(frameRate/2) == 0) {
                        aktMovingObject->swapImage();
                    }
                    aktMovingObject->setFramesDirection(aktMovingObject->getFramesDirection()+1);
                }
                //im letzten Frame rückwärtsgelaufen?
                else if(aktMovingObject->getSpeedX()< 0 ) {
                    if(aktMovingObject->getFramesDirection() > 0) {
                        aktMovingObject->setFramesDirection(0);
                        aktMovingObject->flipHorizontal();
                    }
                    if(aktMovingObject->getFramesDirection()%10 == 0) {
                      aktMovingObject->swapImage();
                    }
                    aktMovingObject->setFramesDirection(aktMovingObject->getFramesDirection()-1);
                }
                else {
                    aktMovingObject->setFramesDirection(0);
                }
            }

            //Positionsaktualisierung aller MovingObjects
            aktMovingObject->setPos(aktMovingObject->getPosX() - 0.5*aktMovingObject->getLength(), yOffset - aktMovingObject->getPosY() - aktMovingObject->getHeight());
        }
    }

    //View wird wieder auf den Spieler zentriert
    window->centerOn(playerObjPointer->getPosX() + 512 - 100 - 0.5 * playerObjPointer->getLength(), 384);
}

/**
 * @brief Game::loadLevelFile
 * @param fileSpecifier
 * Diese Funktion liest Level-Dateien aus und kommt mit wenig Parametern aus.
 * Der Player braucht posX und posY.
 * Enemies brauchen posX, posY und speedX.
 * Obstacles brauchen posX und posY.
 * Planes (Zwischenebenen) brauchen posX und posY.
 * PowerUps brauchen posX, posY und die jeweiligen Boni.
 * @author Simon
 */

void Game::loadLevelFile(QString fileSpecifier) {
    // Spezifizierte Datei öffnen
    QFile levelFile(fileSpecifier);
    if (!levelFile.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "Datei konnte nicht geöffnet werden!";
    } else {
        // Die Datei wurde erfolgreich geöffnet
        // Die Liste wird geleert
        levelSpawn.clear();

        qDebug() << "Lese levelFile mit vorgesetzten Parametern aus:";

        QTextStream fileStream(&levelFile);
        while (!fileStream.atEnd()) {
            QString line = fileStream.readLine();
            //qDebug() << line;
            // Trenne die aktuelle Zeile nach Komma getrennt auf
            QStringList strlist = line.split(",");

            try {

                if (strlist.at(0) == "Levellänge") {
                    if (strlist.length() != 2) {
                        throw std::string("Ungültiger Eintrag für die Levellänge");
                    } else {
                        // Setze die Levellänge
                        levelLength = strlist.at(1).toInt();
                        qDebug("Levellänge gesetzt: %d",levelLength);
                    }
                }

                if (strlist.at(0) == "Player") {
                    if (strlist.length() != 3) {
                        throw std::string("Ungültiger Player-Eintrag: ");
                    } else {
                        // Erstelle das Spieler-Objekt und setze den playerObjPointer
                        qDebug() << "  Player-Eintrag gefunden.";
                        GameObject *playerObject = new Player(strlist.at(1).toInt(), strlist.at(2).toInt(), 0);
                        playerObjPointer = dynamic_cast<Player*>(playerObject);
                    }
                }

                if (strlist.at(0) == "Tourist") {
                    if (strlist.length() != 4) {
                        throw std::string("Ungültiger Tourist-Eintrag:");
                    } else {
                        qDebug() << "  Tourist-Eintrag gefunden.";
                        GameObject *enemyToAppend = new Enemy(strlist.at(1).toInt(), strlist.at(2).toInt(), strlist.at(3).toInt(), enemy_tourist);
                        levelSpawn.push_back(enemyToAppend);
                    }
                }

                if (strlist.at(0) == "Security") {
                    if (strlist.length() != 3) {
                        throw std::string("Ungültiger Security-Eintrag:");
                    } else {
                        qDebug() << "  Security-Eintrag gefunden.";
                        GameObject *enemyToAppend = new Enemy(strlist.at(1).toInt(), strlist.at(2).toInt(), 0, enemy_security);
                        levelSpawn.push_back(enemyToAppend);
                    }
                }

                if (strlist.at(0) == "Obstacle") {
                    if (strlist.length() != 3) {
                        throw std::string("  Ungültiger Obstacle-Eintrag:");
                    } else {
                        qDebug() << "  Obstacle-Eintrag gefunden.";
                        GameObject *obstacleToAppend = new GameObject(strlist.at(1).toInt(), strlist.at(2).toInt(), obstacle);
                        levelSpawn.push_back(obstacleToAppend);
                    }
                }

                if (strlist.at(0) == "Plane") {
                    if (strlist.length() != 3) {
                        throw std::string("Ungültiger Plane-Eintrag:");
                    } else {
                        qDebug() << "  Plane-Eintrag gefunden.";
                        GameObject *planeToAppend = new GameObject(strlist.at(1).toInt(), strlist.at(2).toInt(), plane);
                        levelSpawn.push_back(planeToAppend);
                    }
                }

                if (strlist.at(0) == "Bier") {
                    if (strlist.length() != 3 ) {
                        throw std::string("Ungültiger Bier-Eintrag:");
                    } else {
                        qDebug() << "  Bier-Eintrag gefunden.";
                        GameObject *powerUpToAppend = new PowerUp(strlist.at(1).toInt(), strlist.at(2).toInt(), beerHealth, beerAlcohol, beerAmmo, 0, beer);
                        levelSpawn.push_back(powerUpToAppend);
                    }
                }

                if (strlist.at(0) == "Hendl") {
                    if (strlist.length() != 3 ) {
                        throw std::string("Ungültiger Hendl-Eintrag:");
                    } else {
                        qDebug() << "  Hendl-Eintrag gefunden.";
                        GameObject *powerUpToAppend = new PowerUp(strlist.at(1).toInt(), strlist.at(2).toInt(), hendlHealth, hendlAlcoholMalus, 0, 0, food);
                        levelSpawn.push_back(powerUpToAppend);
                    }
                }

                if (strlist.at(0) == "PowerUp") {
                    if (strlist.length() != 7 ) {
                        throw std::string("Ungültiger PowerUp-Eintrag:");
                    } else {
                        qDebug() << "  PowerUp-Eintrag gefunden.";
                        GameObject *powerUpToAppend = new PowerUp(strlist.at(1).toInt(), strlist.at(2).toInt(), strlist.at(3).toInt(), strlist.at(4).toInt(), strlist.at(5).toInt(), strlist.at(6).toInt(), food);
                        levelSpawn.push_back(powerUpToAppend);
                    }
                }

                if (strlist.at(0) == "Boss") {
                    if (strlist.length() != 4) {
                        throw std::string("Ungültiger Boss-Eintrag:");
                    } else {
                        qDebug() << "  Boss-Eintrag gefunden.";
                        GameObject *enemyToAppend = new Enemy(strlist.at(1).toInt(), strlist.at(2).toInt(), strlist.at(3).toInt(), BOSS);
                        levelSpawn.push_back(enemyToAppend);
                    }
                }
            }
            catch(std::string s) {
                qDebug("%s %s", s.c_str(), line.toStdString().c_str());
            }

        } // end of while

        levelSpawn.sort(compareGameObjects());

        qDebug() << "Auslesen des levelFile beendet.";
    }
}


/**
 * @brief Game::updateScore
 * Aktualisiert die Score des Spielers. Diese Score wird von der Grafik
 * während des Spiels ausgegeben und am Ende des Spiels in die Highscore
 * aufgenommen.
 * @author Simon
 */
void Game::updateScore() {
    playerScore.distanceCovered = playerObjPointer->getPosX();
    playerScore.enemiesKilled = playerObjPointer->getEnemiesKilled();
    playerScore.alcoholPoints = playerScore.alcoholPoints + (playerObjPointer->getAlcoholLevel() / 100);
    playerScore.totalPoints = playerScore.distanceCovered + playerScore.enemiesKilled + playerScore.alcoholPoints;
//    playerScore.name = "Horstl";
}


/**
 * @brief Game::updateHighScore
 * Diese Funktion liest und aktualisiert die Highscore des Spiels. Als Parameter wird ein std::string mode erwartet.
 * Ist der mode = "write", so wird die aktuelle Highscore unter Berücksichtigung der aktuellen playerScore neu geschrieben.
 * Alle anderen Werte für mode lesen nur die alte Highscore und die des Spielers in die Liste ein, um sie z.B. im Highscore-Menü
 * anzuzeigen.
 * Dazu wird versucht, die Datei "wiesnHighscore.txt" auszulesen. Ist dies nicht möglich,
 * so wurde das Spiel in dem aktuellen Verzeichnis noch nie gestartet.
 * Falls die Datei gefunden und gelesen werden kann, so wird jeder Highscore-Eintrag in die scoreList aufgenommen.
 * Anschließend wird die Liste nach der Summe der Punkte absteigend sortiert, und nur die 10 besten Elemente werden gespeichert.
 * Wird für das aktuelle Spiel eine Score angelegt und in der scoreList gespeichert, so wird dieser Eintrag eingeordnet
 * und gegebenenfalls auch abgespeichert.
 */
void Game::updateHighScore(std::string mode) {
    // scoreList leeren und alte Highscore einlesen
    scoreList.clear();
    std::ifstream input("wiesnHighscore.txt");
    if (!input) {
        qDebug() << "Highscore-Datei nicht vorhanden";
    } else {
        qDebug("Lese Highscore ein...");
        // Highscore-Einträge zeilenweise auslesen und als scoreStruct der Liste hinzufügen
        std::string line;
        while (std::getline(input, line)) {
            QString qline = QString::fromStdString(line);
            QStringList strlist = qline.split(",");
            if (strlist.length() == 5) {
                scoreStruct currentScoreItem = {strlist.at(0).toStdString(), strlist.at(1).toInt(), strlist.at(2).toInt(), strlist.at(3).toInt(), strlist.at(4).toInt()};
                scoreList.push_back(currentScoreItem);
            }
        }
    }
    // Datei schließen
    input.close();

    // Score sortieren
    scoreList.sort(compareScores());

    if (mode == "write") {
        // Neue Highscore schreiben

        // Aktuelle Spielerscore hinzufügen und sortieren
        scoreList.push_back(playerScore);
        scoreList.sort(compareScores());

        std::ofstream ofs;
        ofs.open("wiesnHighscore.txt", std::ofstream::out | std::ofstream::trunc);

        int i = 0;
        // Schreibe maximal die besten 10 Scores in die Highscore-Datei
        while (!scoreList.empty() && (i < 10)) {
            scoreStruct currentScore = *scoreList.begin();
            scoreList.pop_front();

            // Highscore-Eintrag schreiben
            ofs << currentScore.name.c_str() << "," << currentScore.totalPoints << "," << currentScore.distanceCovered << "," << currentScore.alcoholPoints << "," << currentScore.enemiesKilled << "\n";
            i++;
        }
        // Datei schließen, damit Änderungen gespeichert werden
        ofs.close();
        qDebug("Highscore geschrieben.");
    }
}

/** gibt stepIntervall zurück
 * wird für Zeit auslesen gebraucht
 * @return int Stepintervall in ms
 * @author Rupert
 */
int Game::getStepIntervall() {
    return stepIntervall;
}

/**
 * @brief setzt den Spielstatus
 * @param newState
 * @author Rupert
 */
void Game::setState(enum gameState newState) {
   state = newState;
    switch(state) {
        case gameIsRunning:
            aktMenu = NULL;
            break;
        case gameMenuStart:
            aktMenu = menuStart;
            break;
        case gameMenuLevel:
            aktMenu = menuLevel;
            break;
        case gameMenuCredits:
            aktMenu = menuCredits;
            break;
        case gameMenuBreak:
            aktMenu = menuBreak;
            break;
        case gameMenuStatisitcs:
            aktMenu = menuStatistics;
            break;
        case gameMenuName:
            aktMenu = menuName;
            break;
        case gameMenuHighscore:
            aktMenu = menuHighscore;
            break;
    }
}

/**
 * @brief Initialisierung der Menüs
 * wird in start() aufgerufen
 * Logik:
 *  Startmenü
 *      Credits
 *  Levelauswahl
 *  spielen...
 *      Pause
 *  Name eingeben
 *  Spielstatistik
 *  Highscore
 *  Von vorne
 *
 * @author Rupert
 */
void Game::menuInit() {
    menuStart = new Menu(new std::string("Wiesn-Run"));
    menuStart->addEntry("Pack ma's!",menuStartId_NewGame,true, gameMenuLevel);
    menuStart->addEntry("Mia san Mia", menuStartId_Credits,true,gameMenuCredits);
    menuStart->addEntry("Pfiat di!", menuStartId_EndGame,true);
    menuStart->displayInit();

    menuCredits = new Menu(new std::string("Credits"));
    menuCredits->addEntry("Grundkurs C++", menuId_NonClickable,false);
    menuCredits->addEntry("da Simon, da Rupi,", menuId_NonClickable,false);
    menuCredits->addEntry("da Felix,", menuId_NonClickable,false);
    menuCredits->addEntry("da Flo und da Johann", menuId_NonClickable,false);
    menuCredits->addEntry("weg do!", menuCreditsId_Back,true,gameMenuStart);
    menuCredits->displayInit();

    menuLevel = new Menu(new std::string("Wos mogst?"));
    menuLevel->addEntry("Demolevel",menuLevelId_Demo, true);
    menuLevel->addEntry("Level 1",menuLevelId_Level1, true);
    menuLevel->addEntry("Level 2",menuLevelId_Level2, true);
    menuLevel->addEntry("Level 3",menuLevelId_Level3, true);
    menuLevel->addEntry("naaa..",menuLevelId_Back, true,gameMenuStart);
    menuLevel->displayInit();

    menuBreak = new Menu(new std::string("Brotzeit!"));
    menuBreak->addEntry("weida",menuBreakId_Resume,true);
    menuBreak->addEntry("koa Lust mehr",menuBreakId_EarlyEnd, true,gameMenuName);
    menuBreak->addEntry("vo vorn!",menuBreakId_EndGame,true);
    menuBreak->displayInit();

    menuStatistics = new Menu(new std::string("Boah!"), Menu::menuType::highscore);
    menuStatistics->addEntry("weida",menuStatisticsId_Next,true,gameMenuHighscore);
    menuStatistics->displayInit();

    menuName = new Menu(new std::string("Wia hoaßt du?"), Menu::menuType::highscore);
    menuName->addEntry("weida",menuNameId_Next,true,gameMenuStatisitcs);
    menuName->displayInit();

    menuHighscore = new Menu(new std::string("Highscores"), Menu::menuType::highscore);
    menuHighscore->addEntry("weida",menuHighscoreId_Next,true,gameMenuStart);
    menuHighscore->displayInit();
}

/**
 * @brief füllt das Statistik- und HighscoreMenü
 * löscht das Statistik- und Highscore-Menü und füllt es mit aktuellen Werten
 * @author Rupert
 */
void Game::displayStatistics() {
    using namespace std;    // für std::string

    // Statistik Menü
    menuStatistics->clear();

    string name = "Name: ";
    name.append(playerScore.name);
    string enemies = "Verstorbene: ";
    enemies.append(to_string(playerScore.enemiesKilled));
    string distance = "glafne Meter: ";
    distance.append(to_string(playerScore.distanceCovered));
    string alk = "Promille: ";
    alk.append(to_string(playerScore.alcoholPoints));
    string points = "Punkte:";
    points.append(to_string(playerScore.totalPoints));

    menuStatistics->addEntry(name,menuId_NonClickable,false);
    menuStatistics->addEntry(enemies,menuId_NonClickable,false);
    menuStatistics->addEntry(distance,menuId_NonClickable,false);
    menuStatistics->addEntry(alk,menuId_NonClickable,false);
    menuStatistics->addEntry(points,menuId_NonClickable,false);
    menuStatistics->addEntry("weida",menuStatisticsId_Next,true,gameMenuHighscore);
    menuStatistics->displayInit();

    // Highscore-Menü
    menuHighscore->clear();

    updateHighScore("listefüllen");
    int scoreCount = 0;
    for(std::list<scoreStruct>::iterator it = scoreList.begin(); it != scoreList.end(); ++it) {
        if(scoreCount > 4) break;

        scoreStruct score = *it;
        string scoreStr = score.name;
        scoreStr.append(": \t");
        scoreStr.append(to_string(score.totalPoints));

        menuHighscore->addEntry(scoreStr,menuId_NonClickable);

        scoreCount++;

    }

    menuHighscore->addEntry("weida",menuHighscoreId_Next,true,gameMenuStart);
    menuHighscore->displayInit();

}
