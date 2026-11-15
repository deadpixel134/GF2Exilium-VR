// Reproducible HTML/CSS layout. Uses the exact user-selected official Touch Plus
// illustration. No controller geometry, buttons or hardware details are redrawn.
// Run: node build-controls.mjs <HTML-output-directory>, then render at 1600x1280.
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const here = path.dirname(fileURLToPath(import.meta.url));
const out = path.resolve(process.argv[2] || path.join(here, 'rendered-html'));
await fs.mkdir(out, { recursive: true });
const art = 'data:image/png;base64,' + (await fs.readFile(path.join(here, 'meta-quest3-touch-plus.png'))).toString('base64');
const text = {
  ko: {
    normal: '① 일반 VR 조작', game: '② 왼손 Trigger 게임 조작', left: '왼손', right: '오른손',
    normalSub: '기본 손 역할 · 이동과 회전은 서로 다른 스틱입니다', gameSub: '왼손 검지 Trigger를 누르는 동안 · 놓으면 일반 VR 조작으로 복귀',
    normalLeft: [
      ['스틱', '기울이기: VR 시점 이동', '누르기: 원본 카메라 기준 위치로 복귀'],
      ['X · Y', '일반 모드: 별도 할당 없음'],
      ['옆면 Grip', '짧게 눌렀다 놓기: 보조패널', '1초 누르기: VR 설정 / 돌아가기'],
      ['검지 Trigger', '누르고 있는 동안 게임 조작 모드', '게임 조작 안내 ②: WASD · F · Escape']
    ],
    normalRight: [
      ['스틱', '기울이기: 시야 회전 · 기본 30°', '누르기: 원본 카메라 이동 추종 전환'],
      ['A · B', 'A: 클릭 · 누른 채 드래그', 'B: 뒤로 가기 · 취소'],
      ['옆면 Grip', '누른 채 손 이동: 월드를 잡아 이동', '놓으면 시점 이동 멈춤'],
      ['검지 Trigger', '가리킨 항목 클릭', '누른 채 포인터 이동: 드래그']
    ],
    gameLeft: [
      ['검지 Trigger', '계속 누르고 있기: 게임 조작 모드', '놓기: 일반 VR 조작으로 복귀'],
      ['이동 스틱', '위 W · 아래 S', '왼쪽 A · 오른쪽 D'],
      ['X · Y', 'X: F 키', 'Y: Escape 키'],
      ['보조패널', '닫혀 있어도 게임 조작 가능', '게임 창을 Windows 포그라운드에 두기']
    ],
    gameRight: [
      ['A · B', 'A: F 키', 'B: Escape 키'],
      ['게임 조작 중', '이동 스틱은 VR 이동 대신 WASD', 'X/A와 Y/B는 물리 버튼 기준'],
      ['손 역할 변경', '패널 손을 바꾸면 해당 손의', 'Trigger로 게임 조작 모드 사용'],
      ['일반 모드로 복귀', '패널 손의 검지 Trigger 놓기', 'A/B는 다시 클릭 / 뒤로 가기']
    ],
    tipsNormal: ['스틱 클릭 = 아래로 누르기. 좌우 클릭 기능은 물리적인 손에 고정됩니다.', '상하 이동·미세 조정은 오른손 Grip을 누른 채 손을 움직여 보세요.'],
    tipsGame: ['기본값: 왼쪽 스틱 이동 · 오른쪽 스틱 회전. 패널 손과 이동 손은 별도 설정입니다.', '일반 클릭 / 뒤로 버튼 교환은 게임 모드의 X/A → F, Y/B → Escape를 바꾸지 않습니다.'],
    system: 'Menu · Meta 버튼은 GF2VR 조작에 할당하지 않습니다.', hardware: '검지 Trigger: 앞쪽  |  Grip: 옆면',
    credit: '컨트롤러 원화: Meta Quest Hardware Art · GF2VR 조작 설명 별도'
  },
  en: {
    normal: '① General VR controls', game: '② Left Trigger game-input mode', left: 'LEFT', right: 'RIGHT',
    normalSub: 'Default hand roles · Movement and turning use different sticks', gameSub: 'Hold left index Trigger · Release it to return to normal VR controls',
    normalLeft: [
      ['Thumbstick', 'Tilt: move the VR viewpoint', 'Press: return to source-camera reference'],
      ['X · Y', 'Normal mode: no additional binding'],
      ['Side Grip', 'Tap and release: auxiliary panel', 'Hold 1 second: VR settings / return'],
      ['Index Trigger', 'Hold: game-input mode', 'See guide ② for WASD · F · Escape']
    ],
    normalRight: [
      ['Thumbstick', 'Tilt: turn view · default 30° snap', 'Press: toggle source-camera following'],
      ['A · B', 'A: click / hold and drag', 'B: back / cancel'],
      ['Side Grip', 'Hold + move hand: grab the world', 'Release: stop moving the viewpoint'],
      ['Index Trigger', 'Click the pointed item', 'Hold + move pointer: drag']
    ],
    gameLeft: [
      ['Index Trigger', 'Keep held: game-input mode', 'Release: normal VR controls'],
      ['Movement stick', 'Up W · down S', 'Left A · right D'],
      ['X · Y', 'X: F key', 'Y: Escape key'],
      ['Auxiliary panel', 'Works while the panel is closed', 'Keep the game window focused']
    ],
    gameRight: [
      ['A · B', 'A: F key', 'B: Escape key'],
      ['While in game mode', 'Movement stick: WASD keys', 'X/A · Y/B: physical buttons'],
      ['Swapped hand roles', 'Use Trigger on the panel hand', 'to activate game-input mode'],
      ['Return to normal mode', 'Release the panel-hand index Trigger', 'A/B resume click / back']
    ],
    tipsNormal: ['Stick click means pressing it down. Click functions stay on the physical left/right hands.', 'For vertical or fine movement, hold right Grip and move your hand.'],
    tipsGame: ['Defaults: left stick moves, right stick turns. Panel hand and movement hand are separate settings.', 'Normal click/back swapping does not change game-mode X/A → F and Y/B → Escape.'],
    system: 'Menu / Meta buttons have no GF2VR binding.', hardware: 'Index Trigger: front  |  Grip: side',
    credit: 'Controller art: Meta Quest Hardware Art · GF2VR annotations are separate'
  },
  ja: {
    normal: '① 通常のVR操作', game: '② 左Triggerのゲーム操作', left: '左手', right: '右手',
    normalSub: '初期の手の役割 · 移動と回転は別のスティック', gameSub: '左人差し指Triggerを押している間 · 離すと通常VR操作へ',
    normalLeft: [
      ['スティック', '傾ける：VR視点の移動', '押し込む：元カメラの基準位置へ'],
      ['X · Y', '通常モード：追加の割り当てなし'],
      ['側面Grip', '短く押して離す：補助パネル', '1秒長押し：VR設定 / 戻る'],
      ['人差し指Trigger', '押している間：ゲーム操作モード', 'WASD・F・Escapeは図②を参照']
    ],
    normalRight: [
      ['スティック', '傾ける：視点回転・初期値30°', '押し込む：元カメラ移動の追従切替'],
      ['A · B', 'A：クリック・長押しドラッグ', 'B：戻る・キャンセル'],
      ['側面Grip', '押しながら手を動かす：ワールド移動', '離す：視点の移動を止める'],
      ['人差し指Trigger', '指した項目をクリック', '押しながらポインター移動：ドラッグ']
    ],
    gameLeft: [
      ['人差し指Trigger', '押し続ける：ゲーム操作モード', '離す：通常のVR操作へ'],
      ['移動スティック', '上W・下S', '左A・右D'],
      ['X · Y', 'X：Fキー', 'Y：Escapeキー'],
      ['補助パネル', '閉じていてもゲーム操作可能', 'ゲームウィンドウを最前面にする']
    ],
    gameRight: [
      ['A · B', 'A：Fキー', 'B：Escapeキー'],
      ['ゲーム操作中', '移動スティックはVR移動の代わりにWASD', 'X/AとY/Bは物理ボタン基準'],
      ['手の役割を交換した場合', 'パネル側の手のTriggerで', 'ゲーム操作モードを使う'],
      ['通常モードに戻る', 'パネル側の人差し指Triggerを離す', 'A/Bはクリック / 戻るに戻る']
    ],
    tipsNormal: ['スティッククリックは下へ押し込む操作。左右のクリック機能は物理的な手に固定されます。', '上下移動や微調整には右Gripを押しながら手を動かしてください。'],
    tipsGame: ['初期値は左スティック移動・右スティック回転。パネル側と移動側は別設定です。', '通常のクリック・戻る交換はゲームモードのX/A → F、Y/B → Escapeを変えません。'],
    system: 'Menu・MetaボタンにGF2VR操作は割り当てていません。', hardware: '人差し指Trigger：前側  |  Grip：側面',
    credit: '原画：Meta Quest Hardware Art · GF2VR操作説明は別途追加'
  },
  'zh-CN': {
    normal: '① 普通VR操作', game: '② 左Trigger游戏输入', left: '左手', right: '右手',
    normalSub: '默认手部角色 · 移动与转向使用不同摇杆', gameSub: '按住左食指Trigger期间 · 松开后返回普通VR操作',
    normalLeft: [
      ['摇杆', '倾斜：移动VR视点', '按下：返回原相机参考位置'],
      ['X · Y', '普通模式：无额外绑定'],
      ['侧面Grip', '短按并松开：辅助面板', '按住1秒：VR设置 / 返回'],
      ['食指Trigger', '按住：游戏输入模式', 'WASD、F、Escape请参见图②']
    ],
    normalRight: [
      ['摇杆', '倾斜：转向，默认30°步进', '按下：切换原相机移动跟随'],
      ['A · B', 'A：点击、按住拖动', 'B：返回、取消'],
      ['侧面Grip', '按住并移动手：抓取世界移动', '松开：停止视点移动'],
      ['食指Trigger', '点击指向的项目', '按住并移动指针：拖动']
    ],
    gameLeft: [
      ['食指Trigger', '持续按住：游戏输入模式', '松开：返回普通VR操作'],
      ['移动摇杆', '上W、下S', '左A、右D'],
      ['X · Y', 'X：F键', 'Y：Escape键'],
      ['辅助面板', '关闭时也可使用游戏模式', '使游戏窗口位于Windows前台']
    ],
    gameRight: [
      ['A · B', 'A：F键', 'B：Escape键'],
      ['游戏输入期间', '移动摇杆发送WASD，不移动VR视点', 'X/A和Y/B按物理按键位置'],
      ['交换手部角色后', '使用面板手的Trigger', '进入游戏输入模式'],
      ['返回普通模式', '松开面板手的食指Trigger', 'A/B恢复点击 / 返回']
    ],
    tipsNormal: ['摇杆点击指向下按压。左右点击功能固定在物理左右手上。', '上下移动和微调时，请按住右Grip并移动手。'],
    tipsGame: ['默认左摇杆移动、右摇杆转向；面板手与移动手是独立设置。', '普通点击/返回交换不改变游戏模式的X/A → F、Y/B → Escape。'],
    system: 'Menu / Meta按钮没有GF2VR绑定。', hardware: '食指Trigger：前方  |  Grip：侧面',
    credit: '控制器原画：Meta Quest Hardware Art · GF2VR操作说明另行添加'
  }
};
function escape(s) { return s.replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;'); }
function cards(items) { return items.map(([title, ...body]) => `<section class="card"><h3>${escape(title)}</h3>${body.map(s => `<p>${escape(s)}</p>`).join('')}</section>`).join(''); }
for (const [lang, t] of Object.entries(text)) for (const mode of ['normal', 'game']) {
  const title = t[mode], subtitle = t[mode + 'Sub'];
  const left = t[mode + 'Left'], right = t[mode + 'Right'];
  const tips = mode === 'normal' ? t.tipsNormal : t.tipsGame;
  const html = `<!doctype html><html lang="${lang}"><meta charset="utf-8"><title>GF2VR · ${escape(title)}</title>
<style>
*{box-sizing:border-box}body{margin:0;width:1600px;height:1280px;background:#f6f8fb;color:#162236;font-family:'Malgun Gothic','Yu Gothic','Microsoft YaHei',Arial,sans-serif}
header{height:164px;background:#132d47;color:white;padding:28px 42px}h1{font-size:44px;line-height:1.2;margin:0 0 14px;font-weight:750}header p{font-size:25px;margin:0;color:#d7e8f7}.brand{float:right;font-size:22px;font-weight:700;margin-top:12px;color:#c4e6ff}
.main{height:860px;position:relative;padding:24px 36px}.art{position:absolute;left:422px;top:112px;width:756px;height:auto;mix-blend-mode:multiply}.art-label{position:absolute;left:422px;top:32px;width:756px;text-align:center;font-size:28px;font-weight:700}.hardware{position:absolute;left:402px;bottom:30px;width:796px;text-align:center;font-size:23px;color:#425266;line-height:1.7}
.column{position:absolute;width:364px;top:24px}.column.left{left:36px}.column.right{right:36px}.hand{height:48px;font-size:29px;font-weight:800;letter-spacing:1px;color:#116891;margin:0 0 12px}.right .hand{color:#9d5a04}.card{height:180px;padding:14px 15px;background:#fff;border:1px solid #dbe3eb;border-left:5px solid #087da8;border-radius:10px;margin-bottom:12px;box-shadow:0 2px 4px #112a4010}.right .card{border-left-color:#cf880b}.card h3{font-size:24px;margin:0 0 8px;font-weight:800;line-height:1.4}.card p{font-size:20px;line-height:1.4;margin:0;overflow-wrap:break-word}
footer{margin:0 36px;border-top:2px solid #d5dee8;padding:18px 12px 0;font-size:25px;line-height:1.55}footer p{margin:0 0 5px}.system{font-size:22px;color:#536174}.credit{font-size:19px;color:#657489;margin-top:10px}
[lang=ko] .card p{word-break:keep-all;overflow-wrap:normal}
</style><header><span class="brand">Meta Quest 3 · Touch Plus</span><h1>GF2VR · ${escape(title)}</h1><p>${escape(subtitle)}</p></header>
<main class="main"><div class="art-label">${escape(t.left)} &nbsp; X / Y &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; A / B &nbsp; ${escape(t.right)}</div><img class="art" src="${art}" alt="Meta Quest 3 Touch Plus controllers"><div class="hardware">${escape(t.hardware)}</div><div class="column left"><h2 class="hand">${escape(t.left)}</h2>${cards(left)}</div><div class="column right"><h2 class="hand">${escape(t.right)}</h2>${cards(right)}</div></main>
<footer>${tips.map(s => `<p>${escape(s)}</p>`).join('')}<p class="system">${escape(t.system)}</p><p class="credit">${escape(t.credit)} · developers.meta.com/vr/downloads/package/oculus-controller-art/</p></footer></html>`;
  const name = (mode === 'normal' ? 'vr-controls-' : 'vr-game-input-') + lang;
  await fs.writeFile(path.join(out, name + '.html'), html, 'utf8');
}
console.log(JSON.stringify({ htmlPages: 8, width: 1600, height: 1280, output: out }));
