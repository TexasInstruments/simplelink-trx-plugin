from cgitb import text
from hashlib import sha256
from pathlib import Path
import struct
import sys
import textwrap
from trx.configs import Config, ConfigDatabase


nv_sector = """/* NV sector with configs prepopulated */

#include <stdint.h>

void NV_useFactoryNvContents(void)
{
    /* Empty */
}

__attribute__((section(".nvSector")))
__attribute__((used))
const uint32_t factory_nv_content[] = {
"""

factory_configs = []

config_dir = Path(sys.argv[1])
db = ConfigDatabase(config_dir)

phy_slot = None
for v in sys.argv[2:]:  # .split(';'):
    if phy_slot is None:
        phy_slot = int(v)
    else:
        factory_configs.append((phy_slot, v))
        phy_slot = None

for idx, name in factory_configs:
    config = db.configs[name]
    reference = int.from_bytes(sha256(config.data).digest()[:4], byteorder="little")
    blob = config.as_nv_entry(config_id=idx, reference=reference)
    words = [int.from_bytes(blob[i : i + 4], "little") for i in range(0, len(blob), 4)]
    lines = [words[i : i + 4] for i in range(0, len(words), 4)]
    nv_sector += f"    /* Config slot: {idx}, {name}, {len(words)*4} bytes, reference: {reference:08x} */\n"
    nv_sector += "    " + ",\n    ".join([", ".join([f"0x{word:08X}" for word in line]) for line in lines]) + ",\n"

nv_sector += "};\n\n"

print(nv_sector)
