$ErrorActionPreference = 'Stop'
# Produce an importable replacement from the saved flow; do not deploy it.
$source = 'C:\Users\User\.node-red\flows.json'
$nodes = Get-Content -Raw -LiteralPath $source | ConvertFrom-Json
$join = $nodes | Where-Object id -eq '3b39e9d05697bdb3'
if (!$join) { throw 'Expected SSR join node missing; refusing to patch another flow.' }
# Retain the existing token assignment without displaying its value.
$tokenSetup = ($join.func -split 'if \(!token\)')[0]
$join.func = $tokenSetup + @'
if (!token) { node.error('SUPABASE_REALTIME_TOKEN missing'); return null; }
const ref = String(Date.now());
msg.payload = {
    topic: 'realtime:public:pzem_status', event: 'phx_join', ref, join_ref: ref,
    payload: {
        config: { broadcast: {self:false}, presence:{key:''},
            postgres_changes:[{event:'*',schema:'public',table:'pzem_status',filter:'group_id=eq.rental-house'}] },
        access_token: token
    }
};
node.warn('SSR Realtime join sent ref=' + ref);
return msg;
'@
($nodes | Where-Object id -eq 'a013cc23ea7db12d').action = 'str'
$handler = $nodes | Where-Object id -eq '1f5530b60b4a4c5f'
$handler.func = @'
let e = msg.payload;
try { if (typeof e === 'string') e = JSON.parse(e); }
catch (_) { node.warn('SSR Realtime invalid JSON'); return null; }
if (!e) return null;
if (e.event === 'phx_reply') {
    if (e.topic !== 'phoenix') node.warn('SSR join reply: ' + JSON.stringify(e.payload));
    return null;
}
if (['system','phx_error','phx_close'].includes(e.event)) {
    node.warn('SSR Realtime ' + e.event + ': ' + JSON.stringify(e.payload));
    return null;
}
if (e.event !== 'postgres_changes') return null;
const d = e.payload?.data || e.data || e.payload;
if (!d || d.type === 'DELETE') return null;
if (d.table && d.table !== 'pzem_status') return null;
const r = d.record || d.new;
if (!r || r.group_id !== 'rental-house') return null;
if (typeof r.is_on !== 'boolean') { node.warn('SSR invalid is_on'); return null; }
node.warn('SSR database change -> MQTT ' + r.is_on);
return {topic:'iotmeter/ssr/set', payload:r.is_on ? 'true':'false', qos:1, retain:true};
'@
$tab = 'bbccb9c049c10965'
$extra = @'
[
 {"id":"ssr-heartbeat-timer","type":"inject","name":"SSR heartbeat every 20s","repeat":"20","once":false,"props":[{"p":"payload"}],"wires":[["ssr-heartbeat"]],"x":180,"y":640},
 {"id":"ssr-heartbeat","type":"function","name":"SSR Phoenix heartbeat","func":"return {payload:JSON.stringify({topic:'phoenix',event:'heartbeat',payload:{},ref:String(Date.now())})};","outputs":1,"wires":[["f1f00899c543082f"]],"x":450,"y":640},
 {"id":"ssr-ws-status","type":"status","name":"SSR WebSocket status","scope":["40ecdb176751b304"],"wires":[["ssr-rejoin"]],"x":180,"y":700},
 {"id":"ssr-rejoin","type":"function","name":"Join on connection","func":"node.warn('SSR WebSocket: ' + msg.status.text); return msg.status.text === 'connected' ? {} : null;","outputs":1,"wires":[["3b39e9d05697bdb3"]],"x":450,"y":700}
]
'@ | ConvertFrom-Json
foreach ($n in $extra) { $n | Add-Member z $tab; $nodes = @($nodes | Where-Object id -ne $n.id) + $n }
$output = Join-Path $PSScriptRoot 'flows-ssr-fixed.private.json'
$nodes | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath $output -Encoding utf8
Write-Output 'Prepared replacement export. Contains existing flow credentials: keep private.'
