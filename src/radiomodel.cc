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

#include "radiomodel.h"
#include <chrono>
#include <iostream>

Embedded::RadioModel::RadioModel(EMANE::NEMId id,
                                 EMANE::PlatformServiceProvider * pPlatformService,
                                 EMANE::RadioServiceProvider * pRadioService):
  EMANE::MACLayerImplementor{id, pPlatformService,pRadioService},
  txTimedEventId_{},
  sMessage_{},
  destination_{}
{}
  
Embedded::RadioModel::~RadioModel(){}

void Embedded::RadioModel::initialize(EMANE::Registrar & registrar)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          EMANE::DEBUG_LEVEL,
                          "MACI %03hu Embedded::RadioModel::%s",
                          id_, 
                          __func__);
  
  auto & configRegistrar = registrar.configurationRegistrar();
  
  configRegistrar.registerNonNumeric<std::string>("message",
                                                  EMANE::ConfigurationProperties::REQUIRED,
                                                  {},
                                                  "Message to transmit to attached NEM.");

  configRegistrar.registerNumeric<std::uint16_t>("destination",
                                                 EMANE::ConfigurationProperties::DEFAULT,
                                                 {EMANE::NEM_BROADCAST_MAC_ADDRESS},
                                                 "Destination NEM. Default: NEM_BROADCAST_MAC_ADDRESS");
}


    
void Embedded::RadioModel::configure(const EMANE::ConfigurationUpdate & update)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          EMANE::DEBUG_LEVEL,
                          "MACI %03hu Embedded::RadioModel::%s",
                          id_, 
                          __func__);
  
  for(const auto & item : update)
    {
      if(item.first == "message")
        {
          sMessage_ = item.second[0].asString();
          
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  EMANE::INFO_LEVEL, 
                                  "MACI %03hu Embedded::RadioModel::%s %s: %s",
                                  id_, 
                                  __func__, 
                                  item.first.c_str(), 
                                  sMessage_.c_str());
        }
      else if(item.first == "destination")
        {
          destination_ = item.second[0].asUINT16();
          
          LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                                  EMANE::INFO_LEVEL, 
                                  "MACI %03hu Embedded::RadioModel::%s %s: %hu",
                                  id_, 
                                  __func__, 
                                  item.first.c_str(), 
                                  destination_);
        }
    }
        
}

void Embedded::RadioModel::start()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          EMANE::DEBUG_LEVEL,
                          "MACI %03hu Embedded::RadioModel::%s",
                          id_, 
                          __func__);
}
    
void Embedded::RadioModel::postStart()
{
   LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                           EMANE::DEBUG_LEVEL,
                           "MACI %03hu Embedded::RadioModel::%s",
                           id_, 
                          __func__);
   
  // we are just using a simple timer to generate/process input
  // coming from an upper waveform layer.
  // 
  // most likely you will want to create a thread and engage in some
  // form of RPC or other messaging protocol
  txTimedEventId_ = 
    pPlatformService_->timerService().
    scheduleTimedEvent(EMANE::Clock::now() + std::chrono::seconds{1},
                       nullptr,
                       std::chrono::seconds{1});
}

void Embedded::RadioModel::stop()
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          EMANE::DEBUG_LEVEL,
                          "MACI %03hu Embedded::RadioModel::%s",
                          id_, 
                          __func__);
   
  // cleanup related to  timer service usage
  if(txTimedEventId_ != 0)
    {
      pPlatformService_->timerService().cancelTimedEvent(txTimedEventId_);
      
      txTimedEventId_ = 0;
    }
}
    
void Embedded::RadioModel::destroy() noexcept
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          EMANE::DEBUG_LEVEL,
                          "MACI %03hu Embedded::RadioModel::%s",
                          id_, 
                          __func__);
}

void Embedded::RadioModel::processUpstreamPacket(const EMANE::CommonMACHeader & ,
                                                 EMANE::UpstreamPacket & pkt,
                                                 const EMANE::ControlMessages &)
{
  LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          EMANE::DEBUG_LEVEL, 
                          "MACI %03hu Embedded::RadioModel::%s message from %hu: %s",
                          id_, 
                          __func__, 
                          pkt.getPacketInfo().getSource(),
                          std::string(static_cast<const char *>(pkt.get()),pkt.length()).c_str());

  std::cout<<std::string(static_cast<const char *>(pkt.get()),pkt.length())<<std::endl;
}
  
void Embedded::RadioModel::processUpstreamControl(const EMANE::ControlMessages &)
{}


void Embedded::RadioModel::processDownstreamControl(const EMANE::ControlMessages &)
{}
  
        
void Embedded::RadioModel::processDownstreamPacket(EMANE::DownstreamPacket & pkt,
                                                   const EMANE::ControlMessages &)
{}

void Embedded::RadioModel::processTimedEvent(EMANE::TimerEventId,
                                             const EMANE::TimePoint &,
                                             const EMANE::TimePoint &,
                                             const EMANE::TimePoint &,
                                             const void *)
{

  EMANE::DownstreamPacket pkt({id_,destination_,0,EMANE::Clock::now()},
                              sMessage_.c_str(),
                              sMessage_.size());
  
  sendDownstreamPacket(EMANE::CommonMACHeader{65533,10},pkt);
};
