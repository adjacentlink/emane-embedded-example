/*
 * Copyright (c) 2018 - Adjacent Link LLC, Bridgewater, New Jersey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of Adjacent Link LLC nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <iostream>

#include <emane/application/logger.h>
#include <emane/application/nembuilder.h>
#include <emane/utils/parameterconvert.h>
#include <emane/exception.h>

#include "radiomodel.h"

#include <mutex>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>

namespace
{
  const char * DEFAULT_MESSAGE = "Hello World!";
  const char * DEFAULT_CONTROL_PORT_ENDPOINT = "0.0.0.0:47000";
  
  std::mutex mutex{};

  void sighandler(int)
  {
    mutex.unlock();
  }
}

int main(int argc, char* argv[])
{
  try
    {
      std::vector<option> options =
        {
          {"help", 0, nullptr, 'h'},
          {"message", 0, nullptr, 'm'},
          {"controlport", 0, nullptr, 'c'},
          {nullptr, 0, nullptr, 0},
        };

      
      int iOption{};
      int iOptionIndex{};
      EMANE::NEMId id{};
      std::string sMessage{DEFAULT_MESSAGE};
      std::string sControlPortEndpoint{DEFAULT_CONTROL_PORT_ENDPOINT};


      while((iOption = getopt_long(argc,argv,"hm:c:", &options[0],&iOptionIndex)) != -1)
        {
          switch(iOption)
            {
            case 'h':
              // --help
              std::cout<<"usage: emane-develop-mhal [OPTIONS]... NEMID"<<std::endl;
              std::cout<<std::endl;
              std::cout<<"parameters:"<<std::endl;
              std::cout<<"  NEMID                             NEM id of node.[1,65534]."<<std::endl;
              std::cout<<std::endl;
              std::cout<<"options:"<<std::endl;
              std::cout<<"  -h, --help                        Print this message and exit."<<std::endl;
              std::cout<<"  -m, --message MESSAGE             ASCII message to transmit."<<std::endl;
              std::cout<<"                                     Default:"<<DEFAULT_MESSAGE<<std::endl;
              std::cout<<"  -c, --controlport ENDPOINT        Control Port endpoint."<<std::endl;
              std::cout<<"                                      Default:"<<DEFAULT_CONTROL_PORT_ENDPOINT<<std::endl;
              std::cout<<std::endl;
              return EXIT_SUCCESS;
              break;

            case 'm':
              sMessage = optarg;
              break;

            case 'c':
              sControlPortEndpoint = optarg;
              break;
              
              
            case '?':
              if(optopt == 't')
                {
                  std::cerr<<"option -"<<static_cast<char>(optopt)<<" requires an argument."<<std::endl;
                }
              else if(isprint(optopt))
                {
                  std::cerr<<"unknown option -"<<static_cast<char>(optopt)<<"."<<std::endl;
                }
              else
                {
                  std::cerr<<"bad option"<<std::endl;
                }
              return EXIT_FAILURE;
            }
        }
      
      if(optind >= argc)
        {
          std::cerr<<"Missing parameters. See `emane-develop-mhal --help`."<<std::endl;
          return EXIT_FAILURE;
        }
      else
        {
          id = EMANE::Utils::ParameterConvert{ argv[optind]}.toINT16(1);
        }

      // create and EMANE logger and set the initial level
      EMANE::Application::Logger logger;
      
      logger.setLogLevel(EMANE::DEBUG_LEVEL);
      
      // create an NEM builder
      EMANE::Application::NEMBuilder nemBuilder{};

      // create an NEMLayers instance to hold the layers of yoru NEM
      EMANE::Application::NEMLayers layers{};

      // create an instance of YOUR radio model
      auto items =
        nemBuilder.buildMACLayer_T<Embedded::RadioModel>(id,
                                                         "embeddedradiomodel",
                                                         {
                                                           {"message",{sMessage}}
                                                         },
                                                         false);

      // store a borrowed reference to your radio model instance
      Embedded::RadioModel * pRadioModel{std::get<0>(items)};

      (void) pRadioModel;

      // add your radio model (as an NEMLayer) to your NEM layers
      layers.push_back(std::move(std::get<1>(items)));
      
      // create and add to layers an emulator physical layer instance
      layers.push_back(nemBuilder.buildPHYLayer(id,
                                                "",
                                                {
                                                  {"fixedantennagain",{"0.0"}},
                                                    {"fixedantennagainenable",{"on"}},
                                                      {"bandwidth",{"1M"}},
                                                        {"noisemode",{"all"}},
                                                          {"propagationmodel",{"precomputed"}},
                                                            {"systemnoisefigure",{"7.0"}},
                                                              {"subid",{"65533"}}
                                                },
                                                false));

      // create a list of all NEMs in your application (there should only be one)
      EMANE::Application::NEMs nems{};
      
      nems.push_back(nemBuilder.buildNEM(id,
                                         layers,
                                         {},
                                         false));

      // create application instance UUID 
      uuid_t uuid;
      uuid_generate(uuid);

      // create an NEM manager to control the emulator state
      // transitions and specifiy all the configuration traditionally
      // part of the platform XML
      auto pNEMManager =
        nemBuilder.buildNEMManager(uuid,
                                   nems,
                                   {
                                     {"otamanagerchannelenable",{"on"}},
                                       {"otamanagergroup",{"224.1.2.8:45702"}},
                                         {"otamanagerloopback",{"on"}},
                                           {"otamanagerdevice",{"lo"}},
                                             {"eventservicegroup",{"224.1.2.8:45703"}},
                                               {"eventservicedevice",{"lo"}},
                                                 {"controlportendpoint",{sControlPortEndpoint}}
                                   });

      // start the NEM manager
      pNEMManager->start();
      
      // post-start the NEM manager
      pNEMManager->postStart();

      struct sigaction action;

      memset(&action,0,sizeof(action));

      action.sa_handler = sighandler;

      sigaction(SIGINT,&action,nullptr);
      sigaction(SIGQUIT,&action,nullptr);

      mutex.lock();

      // signal handler unlocks mutex
      mutex.lock();

      //  applciation work...
      
      // stop the  NEM manager
      pNEMManager->stop();
      
      // destroy the NEM manager
      pNEMManager->destroy();

      std::cout<<"fin"<<std::endl;
    }
  catch(EMANE::Exception & exp)
    {
      std::cerr<<exp.what()<<std::endl;
      return EXIT_FAILURE;
    }
  
  return EXIT_SUCCESS;
}
