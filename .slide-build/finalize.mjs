import fs from 'node:fs/promises';
import {finalizePresentation} from 'file:///C:/Users/dalmo/.codex/plugins/cache/openai-primary-runtime/presentations/26.905.11957/skills/presentations/container_tools/artifact_tool_utils.mjs';
const root='C:/Users/dalmo/ufes/10_periodo/tcc';const skill='C:/Users/dalmo/.codex/plugins/cache/openai-primary-runtime/presentations/26.905.11957/skills/presentations';
const result=await finalizePresentation({workspaceDir:root,candidatePath:root+'/.slide-build/candidate.pptx',finalPath:root+'/outputs/Greedy_Ulysses16_Final.pptx',pythonExecutable:'C:/Users/dalmo/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',integrityValidatorPath:skill+'/container_tools/inspect_presentation_package_integrity.py',layoutValidatorPath:skill+'/container_tools/inspect_presentation_layout_geometry.py',layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-heading-fit'],fontPolicy:{basis:'reference',referenceSha256:'ec2084d143a4d52857cc06c24129abbb45c45d04b90cca06e319ac26d3cadd4f',families:['Helvetica Neue'],referencePath:'C:/Users/dalmo/.codex/plugins/cache/openai-curated-remote/openai-templates/0.1.1/skills/artifact-template-operating-review/assets/reference.pptx'},verifyArtifactToolImport:true,receiptPath:root+'/.slide-build/validation-v3.json'});
console.log(JSON.stringify(result));



