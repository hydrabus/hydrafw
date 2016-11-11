/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2015 Nicolas OBERLI
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define BBIO_SPI_HEADER		"SPI1"

void bbio_spi_init_proto_default(t_hydra_console *con);
void bbio_spi_sniff(t_hydra_console *con);
void bbio_mode_spi(t_hydra_console *con);
