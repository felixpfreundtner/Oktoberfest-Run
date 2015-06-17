#include "game.h"
#include "movingobject.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <cmath>

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
 * @param event
 * @author Rupert
 */
void Game::timerEvent(QTimerEvent *event)
{
    step();
    ///@TODO return von step...
}

/**
 * @brief Erstelle Mockup QApplication app mit Widget inputwindow (Eventfilter installiert) und Zeiger keyInputs auf Input Objekt.
 * Um Funktionen der Tastatur Eingabe entwickeln zu können ist ein Qt Widget Fenster nötig.
 * Auf dem Widget wird ein Eventfilter installiert welcher kontinuierlich Tastureingaben mitloggt.
 * Die Eingaben werden in dem Objekt der Input Klasse gespeichert und können über getKeycomb() abgerufen werden.
 * Das Fenster Mockup kann in einer späteren Code Version ersetzt werden.
 * Der Eventfilter muss dann auf dem neuen Widget Objekt installiert werden.
 *
 * Außerdem wird ein Timer gestartet, der in jedem Intervall timerEvent(...) aufruft, wo dann step() aufgerufen wirt.
 * Das ist dann unsere Game-Loop. Der Timer funktioniert auch bei 5ms Intervall noch genau.
 *
 * Hier müssen auch die Sachen rein, die einmahlig beim Starten ausgeführt werden sollen
 * - alles laden, Fenster anzeigen
 * @return Rückgabewert von app.exec()
 * @author Felix, Rupert
 */
int Game::start() {
    // levelInitial laden
    // worldObjects = levelInitial

    // Player erstellen und in worldObjects einfügen

    QWidget inputwindow;
    inputwindow.resize(320, 240);
    inputwindow.show();
    inputwindow.setWindowTitle(QApplication::translate("Game Widget", "Game Widget (Input Test)"));
    qDebug("initialize inputwindow");
    Input *keyInputs = new Input();
    inputwindow.installEventFilter(keyInputs);

    qDebug("Starte Timer mit 500msec-Intervall");
    Game::startTimer(500);

    return appPointer->exec();
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
 *  - Sound ausgeben
 *  - verbleibende Zeit im Slot berechnen (Timer auslesen)
 *  - entsprechend warten
 * goto LOOP
 * @return 0 bei fehlerfreiem Beenden
 * @author Rupert
 */
int Game::step() {
    using namespace std::chrono;

    high_resolution_clock::time_point akt = letzterAufruf;
    letzterAufruf = high_resolution_clock::now();

//    milliseconds akt_ms = duration_cast<milliseconds>(akt);
//    akt_ms.count();
//    last_ms

    std::size_t ms = duration_cast<milliseconds>(letzterAufruf-akt).count();
    std::string msg = "Game::step() | Vergangene Zeit seit letztem step(): " + std::to_string(ms) + "ms";
    qDebug(msg.c_str());
    //qDebug << "Test";
    /*
    appendWorldObjects();
    reduceWorldObjects();
    evaluateInput();
    calculateMovement();
    detectCollision();
    correctMovement();
    handleEvents();
    renderGraphics();
    playSound();
    */

    return 0;
}


/**
 * @brief Game::detectCollision
 * Diese Funktion berechnet die Kollisionen, welche zwischen ObjektA und ObjektB auftreten.
 * Die Kollision wird dabei immer aus Sicht von ObjektA berechnet. D.h. Variablen wie movingRight
 * bedeuten, dass ObjektA sich nach rechts bewegt hat und dabei ObejektB von Links getroffen hat.
 * @todo Auslesen und durchgehen der Liste, setzen der Variablen, Erstellen der Kollisionsevents
 * @author Simon
 */
void Game::detectCollision(std::list<GameObject*> &objToCalculate) {

    for (std::list<GameObject*>::iterator it=objToCalculate.begin(); it != objToCalculate.end(); ++it) {

        // Anfang, Ende, current nicht moving

        MovingObject *currentObject = dynamic_cast<MovingObject*>(*it);
        // Prüfen, ob das aktuelle Objekt überhaupt vom Typ MovingObject ist
        if (currentObject != 0) {

            GameObject *onePrevious = *(std::prev(it));
            GameObject *twoPrevious = *(std::prev(std::prev(it)));
            GameObject *oneAhead = *(std::next(it));
            GameObject *twoAhead = *(std::next(std::next(it)));
            GameObject *possibleCollision[] = {onePrevious, twoPrevious, oneAhead, twoAhead};


            for (int i = 0; i <= 3; i++) {

                int objASpeedX = currentObject->getSpeedX();
                int objASpeedY = currentObject->getSpeedY();

                bool movingRight;
                bool movingDown;
                int overlapX;
                int overlapY;

                int objBmaxX = possibleCollision[i]->getPosX() + (possibleCollision[i]->getLength() / 2);
                int objBminX = possibleCollision[i]->getPosX() - (possibleCollision[i]->getLength() / 2);
                int objBmaxY = possibleCollision[i]->getPosY() + (possibleCollision[i]->getHeight() / 2);
                int objBminY = possibleCollision[i]->getPosY() - (possibleCollision[i]->getHeight() / 2);
                int objAmaxX = currentObject->getPosX() + (currentObject->getLength() / 2);
                int objAminX = currentObject->getPosX() - (currentObject->getLength() / 2);
                int objAmaxY = currentObject->getPosY() + (currentObject->getHeight() / 2);
                int objAminY = currentObject->getPosY() - (currentObject->getHeight() / 2);

                bool horizontalCollision = false;

                // Check whether collision happend from left
                if (objASpeedX > 0) {
                    movingRight = true;
                } else {
                    movingRight = false;
                }

                // Check whether collision happend from above
                if (objASpeedY < 0) {
                    movingDown = true;
                } else {
                    movingDown = false;
                }

                // Calculate overlap in the X coordinate
                if (movingRight == true) {
                    overlapX = objAmaxX - objBminX;
                } else {
                    overlapX = objBmaxX - objAminX;
                }

                // Calculate overlap in the Y coordinate
                if (movingDown == true) {
                    overlapY = objBmaxY - objAminY;
                } else {
                    overlapY = objAmaxY - objBminY;
                }

                // Detect dominant collision direction
                if (overlapX > overlapY) {
                    horizontalCollision = true;
                } else {
                    horizontalCollision = false;
                }
            }
        }
    }
}
