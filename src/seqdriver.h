/*!
 * @file seqdriver.h
 * @brief Header for the SeqDriver class
 */
#ifndef SEQDRIVER_H
#define SEQDRIVER_H

#include <QWidget>
#include <QThread>
#include <alsa/asoundlib.h>

#include "jacksync.h"
#include "midiarp.h"
#include "midilfo.h"
#include "midiseq.h"
#include "main.h"

/*! @brief ALSA sequencer backend QThread class. Also creates JackSync
 *
 * SeqDriver is created by ArpData at the moment of program start. Its
 * constructor registers ALSA seq input port and the requested number of
 * output ports. I also creates a JackSync instance whose ports are only
 * created when the SeqDriver::setUseJackTransport member is called.
 * Pointers to the MIDI workers MidiLfo, MidiSeq are passed to SeqDriver
 * as arguments.
 * The SeqDriver::run() thread is the ALSA sequencer "callback" process
 * handling all incoming and outgoing sequencer events.
 * When the SeqDriver::setQueueStatus() member is called with True argument,
 * a so called "echo event" is scheduled with zero time. Echo events go back
 * to the callback process and allow output and reception of sequencer
 * events depending on the ALSA queue timing. Depending on the event types,
 * the MIDI worker interfaces are called in series and return their
 * data to be output to the queue. After the data output, a new echo
 * event is requested for the next MIDI event to be output, which will
 * again call the SeqDriver::run() thread, and so on.
 * In order to provide accurate synchronization with external sources
 * such as Jack Transport or an incoming ALSA MIDI clock,
 * SeqDriver works with snd_seq_real_time timing information when it
 * communicates with the ALSA queue. Internally, the real time information
 * is rescaled to a simpler tick-based timing, which is currently 192 tpqn
 * using the SeqDriver::deltaToTick and SeqDriver::tickToDelta functions.
 */
class SeqDriver : public QThread {

    Q_OBJECT

    private:
        int portCount;
        QList<MidiArp *> *midiArpList;
        QList<MidiLfo *> *midiLfoList;
        QList<MidiSeq *> *midiSeqList;
        snd_seq_t *seq_handle;
        int clientid;
        int portid_out[MAX_PORTS];
        int lfoMinPacketSize, lfoPacketSize[20];
        int seqMinPacketSize, seqPacketSize[20];
        int portid_in;
        int queue_id;
        bool startQueue;
        bool modified;
        bool midi_controllable;
        bool threadAbort;
        bool gotKbdTrig;
        int tick, jack_offset_tick, schedDelayTicks;
        int lastSchedTick;
        int lastLfoTick[20], nextLfoTick;
        int lastSeqTick[20], nextSeqTick;
        int nextNoteTick[20], nextArpTick;
        int tempo, internal_tempo;
        QVector<Sample> lfoData;
        Sample seqSample;

        double tickToDelta(int tick);
        int deltaToTick (double curtime);
        double aTimeToDelta(snd_seq_real_time_t* atime);
        const snd_seq_real_time_t* deltaToATime(double curtime);
        void calcClockRatio();

        void schedEvent(MidiEvent outEv, int n_tick, int outport, int length = 0);
        bool requestEchoAt(int echoTick, int infotag = 1);
        bool handleEvent(MidiEvent inEv);
        void handleEcho(MidiEvent inEv);
        void resetTicks();

        JackSync *jackSync;
        jack_position_t jpos;

        int midiTick;
        double m_ratio;         /* duration of one tick, in nanoseconds; based on current tempo */
        snd_seq_real_time_t delta, real_time;
        snd_seq_real_time_t tmptime;

    public:
        bool forwardUnmatched, runQueueIfArp, runArp;
        int portUnmatched;
        bool use_midiclock, use_jacksync, trigByKbd;

    public:
/*! @param p_midiArpList List of pointers to each MidiArp worker
 *  @param p_midiLfoList List of pointers to each MidiLfo worker
 *  @param p_midiSeqList List of pointers to each MidiSeq worker
 *  @param parent QWidget ID of the parent Widget
 */
        SeqDriver(QList<MidiArp*> *p_midiArpList,
                QList<MidiLfo *> *p_midiLfoList,
                QList<MidiSeq *> *p_midiSeqList, int p_portCount, QWidget* parent=0);
        ~SeqDriver();
        int getPortCount();
        void get_time();
        bool isModified();
        void setModified(bool);
        int getAlsaClientId();
        void run();

   signals:
        void midiEvent(int type, int data, int channel, int value);
        void controlEvent(int ccnumber, int channel, int value);
        void noteEvent(int note, int velocity);
        void jackShutdown(bool); //boolean is passed to main toolbar
                                //jackSync button

   public slots:
        void setForwardUnmatched(bool on);
        void setPortUnmatched(int id);
        void setQueueStatus(bool run);
        void setQueueTempo(int bpm);
        void setUseMidiClock(bool on);
        void setMidiControllable(bool on);
        void setUseJackTransport(bool on);
        void jackShutdown();
};

#endif
