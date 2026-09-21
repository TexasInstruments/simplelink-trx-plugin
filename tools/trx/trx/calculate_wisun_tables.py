import sys
from trx.wisun import RegulatoryDomain
import pandas as pd

header = """
/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


"""

###############################################################################################
# EXCEL Export
###############################################################################################
with pd.ExcelWriter("wisun_frequency_tables.xlsx") as writer:
    for region in [RegulatoryDomain(x) for x in sys.argv[1:]]:
        df = region.get_frequency_table_as_dataframe().to_excel(writer, sheet_name=region.name, index=False)

for region in [RegulatoryDomain(x) for x in sys.argv[1:]]:
    with pd.ExcelWriter(f"wisun_delta_frequencies_{region.name}.xlsx") as writer:
        region.get_frequency_table_as_dataframe().to_excel(writer, sheet_name="Frequencies", index=False)
        for id, table in region.get_delta_tables().items():
            table.as_dataframe().to_excel(writer, sheet_name=f"From ChannelPlanId {id}", index=False)

###############################################################################################
# C Export
###############################################################################################

for region in [RegulatoryDomain(x) for x in sys.argv[1:]]:
    with open(f"wisun_delta_tables_{region.name}.h", "w") as f:
        f.write(header)
        f.write(f"#ifndef WISUN_DELTA_TABLES_{region.name}_H\n")
        f.write(f"#define WISUN_DELTA_TABLES_{region.name}_H\n\n")
        f.write("#include <stdint.h>\n\n")
        for id, table in region.get_delta_tables().items():
            f.write(f"// DeltaTable for {region.name}, ChannelPlanID {id}\n")
            f.write(f"extern const uint32_t wisun_delta_table_{region.name}_from_ChannelPlanID{id}[];\n\n")
        f.write(f"/*****************************************\n")
        f.write(f" * HOST FUNCTIONS\n")
        f.write(f" ****************************************/\n\n")
        f.write(f"#define WISUN_DELTA_TABLE_BASEINDEX_INVALID -1\n\n")
        f.write(f"const uint32_t* get_wisun_delta_table_{region.name}(uint16_t channelPlanId);\n")
        f.write(f"int16_t get_wisun_delta_table_baseindex_{region.name}(uint16_t channelPlanId, uint16_t channel);\n\n")
        f.write(f"#endif // WISUN_DELTA_TABLES_{region.name}_H\n\n")
    with open(f"wisun_delta_tables_{region.name}.c", "w") as f:
        f.write(header)
        f.write(f'#include "wisun_delta_tables_{region.name}.h"\n\n')
        for id, table in region.get_delta_tables().items():
            print(f"Region {region.name}, ChannelPlanID {id}: min-delta={table.get_min()}, max-delta={table.get_max()}")
            configlist = table.as_config_list("CC1407")
            entries = configlist.as_commented_list()
            size = len(entries) * 4
            if size > 128 - 4 - 4 - 2:
                print(f"WARNING: DeltaTable might be too big for a single SPIBlock ({size})")
                print(f"    Region {region.name}, ChannelPlanID {id}")
            f.write(f"// DeltaTable for {region.name}, ChannelPlanID {id}\n")
            f.write(f"// Size in bytes: {size}\n")
            f.write(f"const uint32_t wisun_delta_table_{region.name}_from_ChannelPlanID{id}[] =\n")
            f.write("{\n")
            for l, c in entries[:-1]:
                f.write(f"    0x{int.from_bytes(l, byteorder='little'):08x},    //  {c}\n")
            # special handling of last line :(
            l, c = entries[-1]
            f.write(f"    0x{int.from_bytes(l, byteorder='little'):08x}     //  {c}\n")
            f.write("};\n\n")
        # Host functions
        f.write(f"/*****************************************\n")
        f.write(f" * HOST FUNCTIONS\n")
        f.write(f" ****************************************/\n")
        f.write(f"const uint32_t* get_wisun_delta_table_{region.name}(uint16_t channelPlanId)\n")
        f.write("{\n")
        f.write("    switch (channelPlanId)\n")
        f.write("    {\n")
        for id, table in region.get_delta_tables().items():
            f.write(f"        case {id}: return wisun_delta_table_{region.name}_from_ChannelPlanID{id};\n")
        f.write("        default: return 0;\n")
        f.write("    }\n")
        f.write("}\n\n")
        f.write(f"int16_t get_wisun_delta_table_baseindex_{region.name}(uint16_t channelPlanId, uint16_t channel)\n")
        f.write("{\n")
        f.write("    switch (channelPlanId)\n")
        f.write("    {\n")
        for id, table in region.get_delta_tables().items():
            f.write(f"        // Channel to BaseIndex mapping for {region.name}, ChannelPlanID {id}\n")
            f.write(f"        case {id}: switch (channel)\n")
            f.write("            {\n")
            bid = table.get_baseindex_dict()
            for channel, bi in bid.items():
                f.write(f"                case {channel}: return {bi};\n")
            f.write(f"                // Invalid channel for {region.name}, ChannelPlanID {id}\n")
            f.write("                default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;\n")
            f.write("            }\n")
        f.write("        // Invalid ChannelPlanID\n")
        f.write("        default: return WISUN_DELTA_TABLE_BASEINDEX_INVALID;\n")
        f.write("    }\n")
        f.write("}\n\n")
