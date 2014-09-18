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

#include "types.h"

#ifndef _HYDRANFC_CMD_SNIFF_ISO14443_H_
#define _HYDRANFC_CMD_SNIFF_ISO14443_H_

/* Define for detected_protocol[] */
#define MILLER_MODIFIED_106KHZ  (1)
#define MANCHESTER_106KHZ       (2)
extern const u08_t detected_protocol[256];

extern const u08_t miller_modified_106kb[256];
extern const u08_t manchester_106kb[256];

#endif /* _HYDRANFC_CMD_SNIFF_ISO14443_H_ */
