import { useEffect, useState, useRef } from "react"
import { useHotkeys } from "react-hotkeys-hook"
import { SegmentedControl, Theme, Text, Heading, Card, Flex, ScrollArea, Button, Box, Tabs, IconButton, DropdownMenu, Tooltip, Code, Switch } from "@radix-ui/themes"
import { Editor, type MonacoDiffEditor, type Monaco } from "@monaco-editor/react"
import { KnobPercentage } from "./components/knobs/KnobPercentage"
import Monitor from "./components/monitor/Monitor"
import { getNativeFunction } from "juce-framework-frontend"

declare global {
  interface Window {
    __JUCE__?: {
      backend: {
        addEventListener: (eventId: string, callback: (e: unknown) => void) => void;
      };
      initialisationData: {
        savedState: [ {
            theme?: "light" | "dark",
            selectedTab?: string,
            fileName?: string,
            script?: string,
            hasFileChanged?: boolean,
          },
        ];
      };
    };
  }
}

function App() {
  /// TODO: do saved state for script, output log, output monitor, etc.
  const savedState = window.__JUCE__?.initialisationData.savedState[0];

  const [theme, setTheme] = useState<"light" | "dark">(savedState?.theme || "light");
  const [selectedTab, setSelectedTab] = useState<string>(savedState?.selectedTab || "script");
  const editorRef = useRef<MonacoDiffEditor>(null);
  const outputLogRef = useRef<HTMLDivElement>(null);
  const [fileName, setFileName] = useState<string>(savedState?.fileName || "untitled.lua");
  const [outputLog, setOutputLog] = useState<string[][]>([["0 s"], ["0 s"]]);
  const [hasFileChanged, setHasFileChanged] = useState<boolean>(savedState?.hasFileChanged || false);
  const shouldUpdateOutputLogRef = useRef<boolean>(true);
  const [monitorData, setMonitorData] = useState<number[]>([]);

  /// TODO: load saved state from JSON init data, __JUCE__.backend.initialisationData.savedState
  /// TODO: update saved state via useEffect with each state
  /// TODO: Create react state wrapper? bruh 

  // init
  useEffect(() => {
    if (window.__JUCE__) {
      window.__JUCE__.backend.addEventListener("fileSelect", (e) => {
        const [name, data] = e as [string, string];
        setFileName(name);
        editorRef.current?.setValue(data);
        setHasFileChanged(false);
      });

      window.__JUCE__.backend.addEventListener("outputLogUpdate", (e) => {
        setOutputLog(e as string[][]);
      });

      window.__JUCE__.backend.addEventListener("outputMonitorUpdate", (e) => {
        setMonitorData(e as number[]);
      });

      console.log("Saved state init: ", savedState);
    }

    const interval = setInterval(() => { 
      if (!shouldUpdateOutputLogRef.current)
        return;

      getNativeFunction('requestOutputLog')();
    }, 20);

    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    const state = {
      theme: theme,
      selectedTab: selectedTab,
      fileName: fileName,
      hasFileChanged: hasFileChanged,
    };

    console.log("Saving state:", state);

    getNativeFunction("setSavedState")(state);
  }, [theme, selectedTab, fileName, hasFileChanged]);

  // scroll output log to bottom on update
  useEffect(() => {
    if (!outputLogRef.current)
      return;

    // set scroll to automatically update to bottom
    outputLogRef.current.scrollTop = outputLogRef.current.scrollHeight;
  }, [outputLog]);

  // output monitor update
  useEffect(() => {
    const interval = setInterval(() => {
      getNativeFunction("requestOutputMonitor")();
    }, 50);

    return () => clearInterval(interval);
  }, []);

  function saveFile(useLastOpenedFile: boolean) {
    if (selectedTab !== "script")
      return;

    const fileData = editorRef.current?.getValue() || "";
    getNativeFunction("saveFile")(useLastOpenedFile, fileData);
  }

  useHotkeys('ctrl+s', () => saveFile(true))
  
  /// TODO: Update WebView2 version so that this hotkey is supported (https://github.com/MicrosoftEdge/WebView2Feedback/issues/5153)
  useHotkeys('ctrl+shift+s', () => saveFile(false))

  return (
    <>
      <Theme appearance={theme}>
        <Box p="2">
        <Flex gap="3" direction="column" align="center">
          <Heading align="center">Formulizer Controller</Heading>
          <Box>
          <Flex gap="3" direction="row" align="center" justify="center">
            <Card>
              <ScrollArea type="always" scrollbars="horizontal" size="2" style={{ width: "400px", height: "120px" }}>
                <Flex direction="row" gap="3">
                {Array.from({ length: 64 }).map((_, i) => (
                  <KnobPercentage label={i.toString()} theme="stone"></KnobPercentage>
                ))} 
                </Flex>
              </ScrollArea>
            </Card>
            <Flex gap="3" direction="column" align="center" justify="center">
              <Flex gap="1">
              <DropdownMenu.Root>
                <Tooltip content="Browse Presets">
                  <DropdownMenu.Trigger>
                    <IconButton variant="soft">⏷</IconButton>
                  </DropdownMenu.Trigger>
                </Tooltip>
                <DropdownMenu.Content size="1">
                  <DropdownMenu.Label>Presets</DropdownMenu.Label>
                  <DropdownMenu.Separator />
                  <DropdownMenu.Item>Empty</DropdownMenu.Item>
                  <DropdownMenu.Item>Basic LFO</DropdownMenu.Item>
                </DropdownMenu.Content>
              </DropdownMenu.Root>
              <Tooltip content="Open File">
                <Button highContrast variant="soft" style={{ width: "25vw" }} onClick={() => {
                  getNativeFunction("openFile")();
                }}>
                  {fileName}{hasFileChanged ? "*" : ""}
                </Button>
              </Tooltip>
              <DropdownMenu.Root>
                <Tooltip content="Save File">
                  <DropdownMenu.Trigger>
                    <IconButton variant="surface">🖫</IconButton>
                  </DropdownMenu.Trigger>
                </Tooltip>
                <DropdownMenu.Content>
                  <DropdownMenu.Item shortcut="Ctrl+S"
                    onClick={() => saveFile(true)}>
                    Save
                  </DropdownMenu.Item>
                  <DropdownMenu.Item shortcut="Ctrl+Shift+S"
                    onClick={() => saveFile(false)}>
                      Save As...
                  </DropdownMenu.Item>
                </DropdownMenu.Content>
              </DropdownMenu.Root>
              </Flex>
              <Button onClick={() => { getNativeFunction("compile")(editorRef.current?.getValue()); }}>
                🞂 Compile
              </Button>
            </Flex>
          </Flex>
          </Box>
          <Monitor data={monitorData}/>
          <Tabs.Root defaultValue={selectedTab} onValueChange={setSelectedTab}>
            <Tabs.List>
              <Tabs.Trigger value="script">Script{hasFileChanged ? "*" : ""}</Tabs.Trigger>
              <Tabs.Trigger value="output">Output</Tabs.Trigger>
              <Tabs.Trigger value="settings">Settings</Tabs.Trigger>
            </Tabs.List>

            <Box style={{height: "50vh", width: "95vw"}}>
              <Tabs.Content value="script" forceMount style={{ display: selectedTab === "script" ? "block" : "none", height: "100%"}}>
                <Editor
                  height="100%"
                  theme={theme === "light" ? "vs-lite" : "vs-dark"}
                  defaultLanguage="lua"
                  defaultValue={savedState?.script || "print('Hello World!')"}
                  onMount={(editor: MonacoDiffEditor, monaco: Monaco) => {
                    editorRef.current = editor
                    editorRef.current.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, () => saveFile(true))
                    editorRef.current.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.KeyS, () => saveFile(false))
                  }}
                  onChange={() => {
                    if (!hasFileChanged) setHasFileChanged(true);
                    getNativeFunction("setSavedState")({ script: editorRef.current?.getValue() });
                  }}
                >
                </Editor> 
              </Tabs.Content>

              <Tabs.Content value="output" forceMount style={{ display: selectedTab === "output" ? "block" : "none", height: "100%" }}>
                {/* TODO: Fix this not working properly (height bad, cant reach bottom code labels) */}
                <Box style={{padding: 4}}>
                <Flex direction="column">
                  <Flex direction="row" gap="3" justify="center" align="center">
                  <Button onClick={() => {
                    getNativeFunction("clearOutputLog")();
                    setOutputLog([["0 s"], ["0 s"]]);
                    }}>Clear</Button>
                  <Text as="label">
                    <Flex gap="1" direction="row">
                      <Switch onCheckedChange={(v) => {shouldUpdateOutputLogRef.current = v}} defaultChecked></Switch>
                      Enabled
                    </Flex>
                  </Text>
                  </Flex>
                  <Flex direction="row" gap="3" justify="center">
                    <Text size="1">Elapsed Compile Time: {outputLog[0]} s</Text>
                    <Text size="1">Elapsed processBlock Time: {outputLog[1]} s</Text>
                  </Flex>
                </Flex>
                </Box>
                <ScrollArea size="2" ref={outputLogRef} type="always">
                  <Flex direction="column">
                  {
                    outputLog.length == 2 ?
                      <Code color="gray">Output Log is empty...</Code>
                      :
                    outputLog.slice(2).map((message) => (
                      <Code color={message[1] == '1' ? "red" : undefined}>
                        {message[0]}
                      </Code>
                    ))
                  }
                  </Flex>
                </ScrollArea>
              </Tabs.Content>

              <Tabs.Content value="settings" forceMount style={{ display: selectedTab === "settings" ? "block" : "none" }}>
                <SegmentedControl.Root
                  defaultValue={theme}
                  onValueChange={(value) => setTheme(value as "light" | "dark")}>
                  <SegmentedControl.Item value="light">Light</SegmentedControl.Item>
                  <SegmentedControl.Item value="dark">Dark</SegmentedControl.Item>
                </SegmentedControl.Root>
              </Tabs.Content>
            </Box>
          </Tabs.Root>
        </Flex>
        </Box>
      </Theme>
    </>
  )
}

export default App
