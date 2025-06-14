/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod, Julian Cable
 *
 * Description: main programme for console mode
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "DRM.h"
#include "DRM_main.h"

#if defined(__unix__) && !defined(__APPLE__)
# include <csignal>
#endif
#ifdef USE_CONSOLEIO
# include "linux/ConsoleIO.h"
#endif
#include "GlobalDefinitions.h"
#include "DRMReceiver.h"
//#include "DrmTransmitter.h"
//#include "DrmSimulation.h"
#include "util/Settings.h"
#include <iostream>
#include <unistd.h>

void DRM_loop(int rx_chan)
{
    drm_t *drm = DRM_drm_p(rx_chan);
    //printf("$$$$ DRM_loop rx_chan=%d run=%d pid=%d\n", rx_chan, drm->run, getpid());
    assert(drm->init);
    
    CConsoleIO ConsoleIO;
    
    try {
		CSettings Settings;
		Settings.Load(ARRAY_LEN(drm_argv), (char **) drm_argv);
		Settings.Put("Receiver", "samplerateaud", snd_rate);
		
		string mode = Settings.Get("command", "mode", string());

		if (mode != "receive")
        {
            string usage(Settings.UsageArguments());
            for (;;)
            {
                size_t pos = usage.find("$EXECNAME");
                if (pos == string::npos) break;
                usage.replace(pos, sizeof("$EXECNAME")-1, drm_argv[0]);
            }
            cerr << usage << endl << endl;
            kiwi_exit(0);
        }

        //CDRMSimulation DRMSimulation;
        CDRMReceiver DRMReceiver(&Settings);

        //DRMSimulation.SimScript();
        DRMReceiver.LoadSettings();

        while (1) {

            if (!drm->run) {
                //printf("DRM stopped\n");
                while (!drm->run) {
                    DRM_run_sleep();
                    //real_printf("-%d- ", getpid()); fflush(stdout);
                }
                //printf("DRM running\n");
            }
        
            // set the frequency from the command line or ini file
            int iFreqkHz = DRMReceiver.GetParameters()->GetFrequency();
            if (iFreqkHz != -1)
                DRMReceiver.SetFrequency(iFreqkHz);
    
            #ifdef USE_CONSOLEIO
                ConsoleIO.Enter(&DRMReceiver);
            #endif

            ERunState eRunState = RESTART;
            do {
                DRMReceiver.InitReceiverMode();
                DRMReceiver.SetInStartMode();
                eRunState = RUNNING;
                
                do {
                    DRMReceiver.updatePosition();
                    MEASURE_TIME("p", 0, DRMReceiver.process());
    
                    #ifdef USE_CONSOLEIO
                        eRunState = ConsoleIO.Update(drm);
                    #endif

                    if (!drm->run) eRunState = STOPPED;
                } while (eRunState == RUNNING);

                #ifdef USE_CONSOLEIO
                    ConsoleIO.Restart();
                #endif

            } while (eRunState == RESTART);
            
            DRMReceiver.CloseSoundInterfaces();
    
            #ifdef USE_CONSOLEIO
                ConsoleIO.Leave();
            #endif
            
            if (eRunState == STOP_REQUESTED)
                //drm->run = 0;
                return;
        }
    }
	catch(CGenErr GenErr)
	{
        perror(GenErr.strError.c_str());
    }
    catch (string strError)
    {
        perror(strError.c_str());
    }
}

void
DebugError(const char *pchErDescr, const char *pchPar1Descr,
		   const double dPar1, const char *pchPar2Descr, const double dPar2)
{
	FILE *pFile = fopen("test/DebugError.dat", "a");
	fprintf(pFile, "%s", pchErDescr);
	fprintf(pFile, " ### ");
	fprintf(pFile, "%s", pchPar1Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e ### ", dPar1);
	fprintf(pFile, "%s", pchPar2Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e\n", dPar2);
	fclose(pFile);
	fprintf(stderr, "\nDebug error! For more information see test/DebugError.dat\n");
	kiwi_exit(1);
}
