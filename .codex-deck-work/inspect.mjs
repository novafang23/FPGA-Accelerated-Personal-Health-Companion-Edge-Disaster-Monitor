import { pathToFileURL } from 'node:url';
const h = await import(pathToFileURL('C:/Users/abhin/.codex/plugins/cache/openai-primary-runtime/presentations/26.1024.11018/skills/presentations/container_tools/runtime_helpers.mjs').href);
const { FileBlob, PresentationFile } = await h.importRuntimeModule('@oai/artifact-tool');
const p = await PresentationFile.importPptx(await FileBlob.load('C:/Users/abhin/OneDrive/Desktop/verilog/VALOR Explainer Deck.pptx'));
const x = await p.inspect({kind:'slide,textbox,image,shape,notes',maxChars:60000});
console.log(x.ndjson);
