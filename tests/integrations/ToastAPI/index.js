import { toast } from "novadesk";

console.log("=== ToastAPI ===");
console.log("compatible:", toast.isCompatible());

const initialized = toast.initialize({
  appName: "Novadesk",
  companyName: "OfficialNovadesk",
  productName: "Novadesk"
});

console.log("initialized:", initialized);

const id = toast.show({
  title: "Novadesk Toast",
  message: "Toast API is available from the novadesk module.",
  duration: "short",
  actions: ["Open", "Dismiss"]
});

console.log("toast id:", id);
if (id === null) {
  console.log("toast error:", toast.getLastError());
}
