// Tests system.fs API

console.log("=== FS API Test Started ===");

var fs = system.fs;
var baseDir = path.join(__dirname, "tmp_fs_api");
var nestedDir = path.join(baseDir, "nested", "child");
var fileA = path.join(baseDir, "a.txt");
var fileB = path.join(baseDir, "b.txt");
var fileC = path.join(baseDir, "c.txt");
var linkPath = path.join(baseDir, "a.link");

// Cleanup from previous runs.
fs.rm(baseDir, true);

console.log("constants:", JSON.stringify(fs.constants));

console.log("mkdir recursive:", fs.mkdir(nestedDir, true));
console.log("exists baseDir:", fs.exists(baseDir));

console.log("writeFile a:", fs.writeFile(fileA, "Hello"));
console.log("appendFile a:", fs.appendFile(fileA, " World"));
console.log("readFile a:", fs.readFile(fileA));

console.log("copyFile a->b:", fs.copyFile(fileA, fileB));
console.log("rename b->c:", fs.rename(fileB, fileC));
console.log("readFile c:", fs.readFile(fileC));

var fd = fs.open(fileA, "rb+");
console.log("open fd:", fd);
console.log("fstat:", JSON.stringify(fs.fstat(fd)));
console.log("read 5:", fs.read(fd, 5, 0));
console.log("write at 0 bytes:", fs.write(fd, "HELLO", 0));
console.log("fsync:", fs.fsync(fd));
console.log("ftruncate 8:", fs.ftruncate(fd, 8));
console.log("close fd:", fs.close(fd));
console.log("readFile a after fd ops:", fs.readFile(fileA));

console.log("truncate c to 4:", fs.truncate(fileC, 4));
console.log("readFile c after truncate:", fs.readFile(fileC));

console.log("readdir baseDir:", JSON.stringify(fs.readdir(baseDir)));
var dh = fs.opendir(baseDir);
console.log("opendir handle:", dh);
console.log("readdir via handle:", JSON.stringify(fs.readdir(dh)));
console.log("close dir handle:", fs.close(dh));

console.log("stat a:", JSON.stringify(fs.stat(fileA)));
console.log("lstat a:", JSON.stringify(fs.lstat(fileA)));
console.log("access F_OK:", fs.access(fileA, fs.constants.F_OK));
console.log("access R_OK:", fs.access(fileA, fs.constants.R_OK));
console.log("realpath a:", fs.realpath(fileA));

console.log("link a->a.link:", fs.link(fileA, linkPath));
console.log("readFile a.link:", fs.readFile(linkPath));

console.log("utimes a:", fs.utimes(fileA, Date.now(), Date.now()));

var tempDir = fs.mkdtemp("novadesk_fs_");
console.log("mkdtemp:", tempDir, "exists:", fs.exists(tempDir));
if (tempDir) {
    fs.rm(tempDir, true);
}

console.log("unlink a.link:", fs.unlink(linkPath));
console.log("unlink a:", fs.unlink(fileA));
console.log("unlink c:", fs.unlink(fileC));

console.log("rmdir nested child:", fs.rmdir(path.join(baseDir, "nested", "child")));
console.log("rm baseDir recursive:", fs.rm(baseDir, true));
console.log("exists baseDir after rm:", fs.exists(baseDir));

console.log("=== FS API Test Completed ===");
