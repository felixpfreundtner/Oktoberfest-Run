#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include <vector>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QtEndian>
#include<stdio.h>
#include<stdlib.h>
#include <iostream>
#include <string>
#include <QVector>
#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

/**
 * @brief  Die Audio Klasse erzeugt Audioobjekte, welche Wave Samples und deren Meta Information speichern. Für jedes Audio Wave File im Ordner Audio (AudioEventgruppe audioType) wird zum Spielstart in Game über den Konstruktor des AudioControl Objekt audioOutput Audio(std::string type_name) eine Instanz der Klasse erstellt und das erstellte Objekt in audioOutput an eine Liste audioobjects angehängt. Bei der Erstellung liest das Audioobjekt die zum Audio Objekt gehörigen Samples byteweise aus der gespeicherten WAVE Datei in den Vektor samples ein, wobei die Samples normalisiert werden. Das Eingabeformat sind Einkanal little-endian RIFF Wave Dateien mit 16 bit oder 8 bit Bittiefe und einer Samplerate von 44100 Hz. Das audioControl Objekt audioOutput kann auf ein bestimmtes Sample über die Methode getSample(int pos) zugreifen. Die Anzahl aller Samples im Vektor samples kann über getSamplenumber() abgefragt werden.
 * @author  Felix
 */
class Audio{

public:

    // Methoden
    Audio(std::string type_name);
    ~Audio();
    std::string getSource();
    float getSample(int pos);
    int getSamplenumber();

private:

    // Methoden
    void readSamples();
    qint16 to16bitSample(quint8 sample8bit);
    void normalize();

    // Variablen
    /**
     * @var  std::string source
     * @brief  source speichert den Namen des Audioobjekts als string welcher dem Dateinamen der zugehörigen Wave Datei entspricht.
     */
    std::string source;

    /**
     * @var std::vector<float> samples
     * @brief samples speichert die normalisierten samples des Audio Objekts als QVektor mit 32 bit float Werten.
     */
    std::vector<float> samples;
    /**
     * @var  int samplenumber
     * @brief samplenumber speichert die Anzahl an Samples in der gesamten Audio Datei des Audio Objekts als Integer.
     */    
    int samplenumber;

};

#endif // AUDIO_H
