import * as system from "system";

const stats = system.recycleBin.getStats();
console.log(stats.count); // number of items
console.log(stats.size);  // total bytes

system.recycleBin.openBin();
// system.recycleBin.emptyBin();
system.recycleBin.emptyBinSilent();
