/*
 * Carla Native Plugins
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

// for UINT32_MAX
#define __STDC_LIMIT_MACROS
#include <cstdint>

#include "CarlaNative.hpp"
#include "CarlaMIDI.h"
#include "CarlaString.hpp"
#include "CarlaThread.hpp"
#include "RtList.hpp"

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Util.h"

#define WANT_ZYNADDSUBFX_UI

#ifdef WANT_ZYNADDSUBFX_UI
#include "zynaddsubfx/UI/common.H"
#include "zynaddsubfx/UI/MasterUI.h"
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_Dial.H>
#endif

#include <ctime>

#include <set>
#include <string>

// Dummy variables and functions for linking purposes
const char* instance_name = nullptr;
class WavFile;
namespace Nio {
   bool start(void){return 1;}
   void stop(void){}
   bool setSource(std::string){return true;}
   bool setSink(std::string){return true;}
   std::set<std::string> getSources(void){return std::set<std::string>();}
   std::set<std::string> getSinks(void){return std::set<std::string>();}
   std::string getSource(void){return "";}
   std::string getSink(void){return "";}
   void waveNew(WavFile*){}
   void waveStart(void){}
   void waveStop(void){}
   void waveEnd(void){}
}

SYNTH_T* synth = nullptr;

#ifdef WANT_ZYNADDSUBFX_UI
#define PIXMAP_PATH "/usr/share/zynaddsubfx/pixmaps"
#define SOURCE_DIR "/usr/share/zynaddsubfx/examples"

static Fl_Tiled_Image* gModuleBackdrop = nullptr;

void set_module_parameters(Fl_Widget* o)
{
    CARLA_ASSERT(gModuleBackdrop != nullptr);
    o->box(FL_DOWN_FRAME);
    o->align(o->align() | FL_ALIGN_IMAGE_BACKDROP);
    o->color(FL_BLACK);
    o->image(gModuleBackdrop);
    o->labeltype(FL_SHADOW_LABEL);
}
#endif

class ZynAddSubFxPlugin : public PluginDescriptorClass
{
public:
    enum Parameters {
        PARAMETER_COUNT = 0
    };

    ZynAddSubFxPlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          kMaster(new Master()),
          kSampleRate(getSampleRate()),
#ifdef WANT_ZYNADDSUBFX_UI
          fUi(nullptr),
          fUiClosed(0),
#endif
          fThread(kMaster)
    {
        fThread.start();
        maybeInitPrograms(kMaster);
        fThread.waitForStarted();

#ifdef WANT_ZYNADDSUBFX_UI
        fl_register_images();

        Fl_Dial::default_style(Fl_Dial::PIXMAP_DIAL);

        if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/knob.png"))
            Fl_Dial::default_image(img);
        else
            Fl_Dial::default_image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/knob.png"));

        if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/window_backdrop.png"))
            Fl::scheme_bg(new Fl_Tiled_Image(img));
        else
            Fl::scheme_bg(new Fl_Tiled_Image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/window_backdrop.png")));

        if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/module_backdrop.png"))
            gModuleBackdrop = new Fl_Tiled_Image(img);
        else
            gModuleBackdrop = new Fl_Tiled_Image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/module_backdrop.png"));

        Fl::background(50, 50, 50);
        Fl::background2(70, 70, 70);
        Fl::foreground(255, 255, 255);
#endif
    }

    ~ZynAddSubFxPlugin()
    {
#ifdef WANT_ZYNADDSUBFX_UI
        if (fUi != nullptr)
            delete fUi;
#endif

        //ensure that everything has stopped
        pthread_mutex_lock(&kMaster->mutex);
        pthread_mutex_unlock(&kMaster->mutex);
        fThread.stop();

        delete kMaster;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        return PARAMETER_COUNT;
    }

    const Parameter* getParameterInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        //if (index >= PARAMETER_COUNT)
        return nullptr;

        static Parameter param;

        param.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            param.hints = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
            param.name  = "Master Volume";
            param.unit  = nullptr;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.ranges.def = 100.0f;
            break;
#endif
        }

        return &param;
    }

    float getParameterValue(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            return kMaster->Pvolume;
#endif
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount()
    {
        return sPrograms.count();
    }

    const MidiProgram* getMidiProgramInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= sPrograms.count())
            return nullptr;

        const ProgramInfo* const pInfo(sPrograms.getAt(index));

        static MidiProgram midiProgram;
        midiProgram.bank    = pInfo->bank;
        midiProgram.program = pInfo->prog;
        midiProgram.name    = pInfo->name;

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < getParameterCount());

        switch (index)
        {
        }

        return;

        // unused, TODO
        (void)value;
    }

    void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        if (bank >= kMaster->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        bool isOffline = false;

        if (isOffline)
            loadProgram(kMaster, bank, program);
        else
            fThread.loadLater(bank, program);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        // broken
        //for (int i=0; i < NUM_MIDI_PARTS; i++)
        //    kMaster->setController(0, MIDI_CONTROL_ALL_SOUND_OFF, 0);
    }

    void process(float**, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents)
    {
        if (pthread_mutex_trylock(&kMaster->mutex) != 0)
        {
            carla_zeroFloat(outBuffer[0], frames);
            carla_zeroFloat(outBuffer[1], frames);
            return;
        }

        for (uint32_t i=0; i < midiEventCount; i++)
        {
            const MidiEvent* const midiEvent = &midiEvents[i];

            const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
            const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

            if (MIDI_IS_STATUS_NOTE_OFF(status))
            {
                const uint8_t note = midiEvent->data[1];

                kMaster->noteOff(channel, note);
            }
            else if (MIDI_IS_STATUS_NOTE_ON(status))
            {
                const uint8_t note = midiEvent->data[1];
                const uint8_t velo = midiEvent->data[2];

                kMaster->noteOn(channel, note, velo);
            }
            else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
            {
                const uint8_t note     = midiEvent->data[1];
                const uint8_t pressure = midiEvent->data[2];

                kMaster->polyphonicAftertouch(channel, note, pressure);
            }
        }

        kMaster->GetAudioOutSamples(frames, kSampleRate, outBuffer[0], outBuffer[1]);

        pthread_mutex_unlock(&kMaster->mutex);
    }

#ifdef WANT_ZYNADDSUBFX_UI
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show)
    {
        if (show)
        {
            CARLA_ASSERT(fUi == nullptr);

            if (fUi != nullptr)
                return;

            fUiClosed = 0;
            fUi = new MasterUI(kMaster, &fUiClosed);
            fUi->showUI();

            Fl::check();
        }
        else
        {
            CARLA_ASSERT(fUi != nullptr);

            if (fUi == nullptr)
                return;

            delete fUi;
            fUi = nullptr;

            Fl::check();
        }
    }

    void uiIdle()
    {
        CARLA_ASSERT(fUi != nullptr);

        Fl::check();
    }
#endif

    // -------------------------------------------------------------------
    // Plugin chunk calls

    size_t getChunk(void** const data)
    {
        return kMaster->getalldata((char**)data);
    }

    void setChunk(void* const data, const size_t size)
    {
        kMaster->putalldata((char*)data, size);
    }

    // -------------------------------------------------------------------

private:
    struct ProgramInfo {
        uint32_t bank;
        uint32_t prog;
        const char* name;

        ProgramInfo(uint32_t bank_, uint32_t prog_, const char* name_)
          : bank(bank_),
            prog(prog_),
            name(carla_strdup(name_)) {}

        ~ProgramInfo()
        {
            if (name != nullptr)
            {
                delete[] name;
                name = nullptr;
            }
        }

        ProgramInfo() = delete;
        ProgramInfo(ProgramInfo&) = delete;
        ProgramInfo(const ProgramInfo&) = delete;
    };

    class ZynThread : public CarlaThread
    {
    public:
        ZynThread(Master* const master)
            : kMaster(master),
              fQuit(false),
              fChangeNow(false),
              fNextBank(0),
              fNextProgram(0)
        {
        }

        void loadLater(const uint32_t bank, const uint32_t program)
        {
            fNextBank    = bank;
            fNextProgram = program;
            fChangeNow   = true;
        }

        void stop()
        {
            fQuit = true;
            CarlaThread::stop();
        }

    protected:
        void run()
        {
            while (! fQuit)
            {
                if (fChangeNow)
                {
                    loadProgram(kMaster, fNextBank, fNextProgram);
                    fNextBank    = 0;
                    fNextProgram = 0;
                    fChangeNow   = false;
                }
                carla_msleep(10);
            }
        }

    private:
        Master* const kMaster;

        bool     fQuit;
        bool     fChangeNow;
        uint32_t fNextBank;
        uint32_t fNextProgram;
    };

    Master* const  kMaster;
    const unsigned kSampleRate;

#ifdef WANT_ZYNADDSUBFX_UI
    MasterUI* fUi;
    int       fUiClosed;
#endif

    ZynThread fThread;

    static int sInstanceCount;
    static NonRtList<ProgramInfo*> sPrograms;

public:
    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host)
    {
        if (sInstanceCount++ == 0)
        {
            CARLA_ASSERT(synth == nullptr);
            CARLA_ASSERT(denormalkillbuf == nullptr);

            synth = new SYNTH_T();
            synth->buffersize = host->get_buffer_size(host->handle);
            synth->samplerate = host->get_sample_rate(host->handle);
            synth->alias();

            config.init();
            config.cfg.SoundBufferSize = synth->buffersize;
            config.cfg.SampleRate      = synth->samplerate;
            config.cfg.GzipCompression = 0;

            sprng(std::time(nullptr));
            denormalkillbuf = new float[synth->buffersize];
            for (int i=0; i < synth->buffersize; i++)
                denormalkillbuf[i] = (RND - 0.5f) * 1e-16;

            Master::getInstance();
        }

        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;

        if (--sInstanceCount == 0)
        {
            CARLA_ASSERT(synth != nullptr);
            CARLA_ASSERT(denormalkillbuf != nullptr);

            Master::deleteInstance();

            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }

    static void maybeInitPrograms(Master* const master)
    {
        static bool doSearch = true;

        if (! doSearch)
            return;
        doSearch = false;

        pthread_mutex_lock(&master->mutex);

        // refresh banks
        master->bank.rescanforbanks();

        for (uint32_t i=0, size = master->bank.banks.size(); i < size; i++)
        {
            if (master->bank.banks[i].dir.empty())
                continue;

            master->bank.loadbank(master->bank.banks[i].dir);

            for (unsigned int instrument = 0; instrument < BANK_SIZE; instrument++)
            {
                const std::string insName(master->bank.getname(instrument));

                if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                    continue;

                sPrograms.append(new ProgramInfo(i, instrument, insName.c_str()));
            }
        }

        pthread_mutex_unlock(&master->mutex);
    }

    static void loadProgram(Master* const master, const uint32_t bank, const uint32_t program)
    {
        const std::string& bankdir(master->bank.banks[bank].dir);

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&master->mutex);

            master->bank.loadbank(bankdir);

            for (int i=0; i < NUM_MIDI_PARTS; i++)
                master->bank.loadfromslot(program, master->part[i]);

            master->applyparameters(false);

            pthread_mutex_unlock(&master->mutex);
        }
    }

    static void clearPrograms()
    {
        for (auto it = sPrograms.begin(); it.valid(); it.next())
        {
            ProgramInfo* const programInfo(*it);
            delete programInfo;
        }

        sPrograms.clear();
    }

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZynAddSubFxPlugin)
};

int ZynAddSubFxPlugin::sInstanceCount = 0;
NonRtList<ZynAddSubFxPlugin::ProgramInfo*> ZynAddSubFxPlugin::sPrograms;

struct ProgramsDestructor {
    ProgramsDestructor() {}
    ~ProgramsDestructor()
    {
        ZynAddSubFxPlugin::clearPrograms();
    }
} programsDestructor;

// -----------------------------------------------------------------------

static const PluginDescriptor zynAddSubFxDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
#ifdef WANT_ZYNADDSUBFX_UI
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_SYNTH | PLUGIN_HAS_GUI | PLUGIN_USES_CHUNKS | PLUGIN_USES_SINGLE_THREAD),
#else
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS),
#endif
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ ZynAddSubFxPlugin::PARAMETER_COUNT,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_zynaddsubfx()
{
    carla_register_native_plugin(&zynAddSubFxDesc);
}

// -----------------------------------------------------------------------
