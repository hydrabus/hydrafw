/*
    HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _HYDRANFC_H_
#define _HYDRANFC_H_

extern volatile int nb_irq;
extern volatile int irq;
extern volatile int irq_end_rx;

bool hydranfc_init(void);
bool hydranfc_is_detected(void);

void cmd_nfc_vicinity(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_nfc_mifare(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_nfc_dump_regs(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_nfc_sniff_14443A(BaseSequentialStream *chp, int argc, const char* const* argv);

#endif /* _HYDRANFC_H_ */