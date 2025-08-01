import bpy
import math
import bpy_extras
import gpu
import gpu_extras.batch
import copy
import mathutils
import json

# ブレンダーに登録するアドオン情報
bl_info = {
    "name": "レベルエディタ",
    "author": "Ryuhei Komuro",
    "version": (1, 0),
    "blender": (4, 1, 0),
    "location": "",
    "description": "レベルエディタ",
    "warning": "",
    #"support": "TESTING",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object"
}

#################################################################################

# トップバーの拡張メニュー
class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyName"
    bl_description = "拡張メニュー by " + bl_info["author"]

    def draw(self, context):
        self.layout.operator("wm.url_open_preset", text="Manual", icon='HELP')
        self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        self.layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)
        # self.layout.operator(MYADDON_OT_add_collider.bl_idname, text=MYADDON_OT_add_collider.bl_label)
        # TOPBAR_MT_my_menu.draw(self, context) の中に追加
        self.layout.operator("myaddon.add_mesh", text="立方体").type = "CUBE"
        self.layout.operator("myaddon.add_mesh", text="平面").type = "PLANE"
        self.layout.operator("myaddon.add_mesh", text="UV球").type = "SPHERE"
        self.layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)
        self.layout.operator("myaddon.add_mesh", text="円柱").type = "CYLINDER"
        self.layout.operator("myaddon.add_mesh", text="円錐").type = "CONE"


    def submenu(self, context):
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

#################################################################################

# オペレータ 頂点を伸ばす
class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_stretch_vertex"
    bl_label = "頂点を伸ばす"
    bl_description = "頂点座標を引っ張って伸ばします"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
        print("頂点を伸ばしました。")
        return {'FINISHED'}

#################################################################################

# オペレータ ICO球生成
class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_create_object"
    bl_label = "ICO球生成"
    bl_description = "ICO弾を生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        bpy.ops.mesh.primitive_ico_sphere_add()
        print("ICO弾を生成しました。")
        return {'FINISHED'}

#################################################################################

# オペレータ シーン出力
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportします"
    filename_ext = ".json"

    def parse_scene_recursive(self, file, object, level):

        # 深さ分インデントする
        indent = ''
        for i in range(level):
            indent += "\t"

        # オブジェクト名書き込み
        write_and_print(file, indent + "Object: %s" % object.name)
        trans, rot, scale = object.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)

        write_and_print(file, indent + "Trans(%f,%f,%f)" % (trans.x, trans.y, trans.z))
        write_and_print(file, indent + "Rot(%f,%f,%f)" % (rot.x, rot.y, rot.z))
        write_and_print(file, indent + "Scale(%f,%f,%f)" % (scale.x, scale.y, scale.z))

        # カスタムプロパティ 'file_name'
        if "file_name" in object:
            write_and_print(file, indent + "N %s" % object["file_name"])

        # カスタムプロパティ 'collision'
        if "collider" in object:
            write_and_print(file, indent + "C %s" % object["collider"])
            temp_str = indent + "CC %f %f %f"
            temp_str %= (object["collider_center"][0], object["collider_center"][1], object["collider_center"][2])
            write_and_print(file, temp_str)
            temp_str = indent + "CS %f %f %f"
            temp_str %= (object["collider_size"][0], object["collider_size"][1], object["collider_size"][2])
            write_and_print(file, temp_str)

        write_and_print(file, indent + 'END')
        write_and_print(file, '')
        if object.parent:
            write_and_print(self, file, "Parent: %s" % object.parent.name)

        file.write('\n')  # オブジェクトごとに改行を追加

        for child in object.children:
            self.parse_scene_recursive(file, child, level + 1)

    def export(self):
        print("シーン情報出力開始... %r" % self.filepath)

        with open(self.filepath, "wt") as file:
            write_and_print(file, "SCENE")

            for object in bpy.context.scene.objects:
                if object.parent:
                    continue
                self.parse_scene_recursive(file, object, 0)

    def parse_scene_recursive_json(self, data_parent, object, level):

            # シーンのオブジェクト1個分のjsonオブジェクト生成
            json_object = dict()
            # オブジェクト種類
            json_object["type"] = object.type
            # オブジェクト名
            json_object["name"] = object.name
            # ゲーム内に表示するかのフラグ
            if "visible" in object:
                json_object["visible"] = bool(object["visible"])

            # Todo: その他情報をパック
            # あとでコードを書く A

            # オブジェクトのローカルトランスフォームから
            # 平行移動、回転、スケールを抽出
            trans, rot, scale = object.matrix_local.decompose()
            # 回転を Quternion から Euler (3軸出の回転角) に変換
            rot = rot.to_euler()
            # ラジアンから度数法に変換
            rot.x = math.degrees(rot.x)
            rot.y = math.degrees(rot.y)
            rot.z = math.degrees(rot.z)
            # トランスフォーム情報をディクショナリに登録
            transform = dict()
            transform["translation"] = (trans.x, trans.y, trans.z)
            transform["rotation"] = (rot.x, rot.y, rot.z)
            transform["scaling"] = (scale.x, scale.y, scale.z)
            # まとめて1個分のjsonオブジェクトに登録
            json_object["transform"] = transform
            
            # カスタムプロパティ 'filename'
            if "file_name" in object:
                json_object["file_name"] = object["file_name"]

            # カスタムプロパティ 'collider'
            if "collider" in object:
                collider = dict()
                collider["type"] = object["collider"]
                collider["center"] = object["collider_center"].to_list()
                collider["size"] = object["collider_size"].to_list()
                json_object["collider"] = collider
        
            # 1個分のjsonオブジェクトを親オブジェクトに登録
            data_parent.append(json_object)
        
            # Todo: 直接の子供リストを走査
            # あとでコードを書く B
            if len(object.children) > 0:
                # 子ノードリストを作成
                json_object["children"] = list()

                # 子ノードへ進む(深さが1上がる)
                for child in object.children:
                    self.parse_scene_recursive_json(json_object["children"], child, level + 1)


    def export_json(self):
        """JSON形式でファイルに出力"""

        # 保存する情報をまとめるdisc
        json_object_root = dict()

        # ノード名
        json_object_root["name"] = "scene"
        # オブジェクトリストを作成
        json_object_root["objects"] = list()

        # Todo: シーン内の全オブジェクトを走査してパック
        # あとでコードを書く

         # シーン内の全オブジェクトについて
        for object in bpy.context.scene.objects:
            
            # 親オブジェクトがあるものはスキップ(代わりに親から呼び出す)
            if(object.parent):
                continue

            # シーン直下のオブジェクトをルートノード(深さ0)とし、再帰関数で走査
            self.parse_scene_recursive_json(json_object_root["objects"], object, 0)
        
        # オブジェクトをJSON文字列にエンコード (改行・インデント付き)
        json_text = json.dumps(json_object_root, ensure_ascii=False, cls=json.JSONEncoder, indent=4)

        # コンソールに表示してみる
        print(json_text)

        # ファイルをテキスト形式で書き出し用にオープン
        # スコープを抜けると自動的にクローズされる
        with open(self.filepath, "wt", encoding="utf-8") as file:
            
            # ファイルに文字列を書き込む
            file.write(json_text)

    
    def execute(self, context):
        print("シーン情報をExportします")
        self.export_json()
        print("シーン情報をExportしました")
        self.report({'INFO'}, "シーン情報をExportしました")
        return {'FINISHED'}

#################################################################################

# パネル ファイル名
class OBJECT_PT_file_name(bpy.types.Panel):
    """オブジェクトのファイルネームパネル"""
    bl_idname = "OBJECT_PT_file_name"
    bl_label =  "FileName"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    # サブメニューの描画
    def draw(self, context):

        # パネルに項目を追加
        if "file_name" in context.object:
            # 既にプロパティがあれば、プロパティを表示
            self.layout.prop(context.object, '["file_name"]', text=self.bl_label)
        else:
            # プロパティがなければ、プロパティ追加ボタンを表示
            self.layout.operator(MYADDON_OT_add_filename.bl_idname)

        # パネルに項目に追加
        # self.layout.operator(OBJECT_PT_collider.bl_idname, text=OBJECT_PT_collider.bl_label)
        self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        self.layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)
        self.layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)

#################################################################################

# オペレータ カスタムプロパティ['filename']追加
class MYADDON_OT_add_filename(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_filename"
    bl_label = "Filename 追加"
    bl_description = "['file_name']カスタムプロパティを追加します"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):

        # ['file_name']カスタムプロパティを追加
        context.object["file_name"] = ""

        return {"FINISHED"}

#################################################################################

# コライダー描画
class DrawCollider:

    # 描画ハンドル
    handle = None

    # 3Dビューに登場する描画関数
    def draw_collider():
        # 頂点データ
        vertices = {"pos":[]}
        # インデックスデータ
        indices = []
        # 各頂点の、オブジェクトから中心点からのオフセット
        offsets = [
                    [-0.5,-0.5,-0.5], # 左下前
                    [+0.5,-0.5,-0.5], # 右下前
                    [-0.5,+0.5,-0.5], # 左上前
                    [+0.5,+0.5,-0.5], # 右上前
                    [-0.5,-0.5,+0.5], # 左下奥
                    [+0.5,-0.5,+0.5], # 右下奥
                    [-0.5,+0.5,+0.5], # 左上奥
                    [+0.5,+0.5,+0.5], # 右上奥
        ]

        # 立方体のX,Y,Z方向サイズ
        size = [2,2,2]

        # 現在シーンのオブジェクトリストを走査
        for object in bpy.context.scene.objects:

            # コライダープロパティがなければ、描画をスキップ
            if not "collider" in object:
                continue
            
            # 中心点、サイズの変数を宣言
            center = mathutils.Vector((0,0,0))
            size = mathutils.Vector((2,2,2))

            # プロパティから値を取得
            center[0]=object["collider_center"][0]
            center[1]=object["collider_center"][1]
            center[2]=object["collider_center"][2]
            size[0]=object["collider_size"][0]
            size[1]=object["collider_size"][1]
            size[2]=object["collider_size"][2]

            # 追加前の頂点数
            start = len(vertices["pos"])

            # Boxの8頂点分回す
            for offset in offsets:
                # オブジェクトの中心座標をコピー
                pos = copy.copy(center)
                # 中心点を基準に各頂点をずらす
                pos[0]+=offset[0]*size[0]
                pos[1]+=offset[1]*size[1]
                pos[2]+=offset[2]*size[2]
                # ローカル座標からワールド座標に変換
                pos = object.matrix_world @ pos
                # 頂点データリストに座標を追加
                vertices['pos'].append(pos)

                # 前面を構成する辺の頂点インデックス
                indices.append([start+0,start+1])
                indices.append([start+2,start+3])
                indices.append([start+0,start+2])
                indices.append([start+1,start+3])
                # 奥面を構成する辺の頂点インデックス
                indices.append([start+4,start+5])
                indices.append([start+6,start+7])
                indices.append([start+4,start+6])
                indices.append([start+5,start+7])
                # 前と頂点を繋ぐ辺の頂点インデックス
                indices.append([start+0,start+4])
                indices.append([start+1,start+5])
                indices.append([start+2,start+6])
                indices.append([start+3,start+7 ])

        # ビルドインのシェーダーを取得
        shader = gpu.shader.from_builtin("UNIFORM_COLOR")

        # バッチを作成(引数 : シェーダー、トポロジー、頂点データ、インデックスデータ)
        batch = gpu_extras.batch.batch_for_shader(shader, "LINES", vertices, indices = indices)

        # シェーダーのパラメータ設定
        color = [0.5, 1.0, 1.0, 1.0]
        shader.bind()
        shader.uniform_float("color", color)
        # 描画
        batch.draw(shader)

#################################################################################

# オペレータ カスタムプロパティ['collider']追加
class MYADDON_OT_add_collider(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_collider"
    bl_label = "コライダー 追加"
    bl_description = "['collider']カスタムプロパティを追加します"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):

        # ['collider']カスタムプロパティを追加
        context.object["collider"] = "BOX"
        context.object["collider_center"] = mathutils.Vector((0,0,0))
        context.object["collider_size"] = mathutils.Vector((2,2,2))

        return {"FINISHED"}

#################################################################################

# パネル コライダー
class OBJECT_PT_collider(bpy.types.Panel):
    bl_idname = "OBJECT_PT_collider"
    bl_label = "Collider"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    # サブメニューの描画
    def draw(self, context):

        # パネルに項目を追加
        if "collider" in context.object:
            # 既にプロパティがあれば、プロパティを表示
            self.layout.prop(context.object, '["collider"]', text="type")
            self.layout.prop(context.object, '["collider_center"]', text="Center")
            self.layout.prop(context.object, '["collider_size"]', text="Size")

        else:
            # プロパティがなければ、プロパティ追加ボタンを追加
            self.layout.operator(MYADDON_OT_add_collider.bl_idname)

#################################################################################

def write_and_print(file, str):
    print(str)
    file.write(str)
    file.write('\n')

#################################################################################

classes = (
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_export_scene,
    TOPBAR_MT_my_menu,
    MYADDON_OT_add_filename,
    OBJECT_PT_file_name,
    MYADDON_OT_add_collider,
    OBJECT_PT_collider,
    MYADDON_OT_add_mesh,
    MYADDON_OT_add_visibility,
)

#################################################################################

def register():
    # blenderにクラスを登録
    for cls in classes:
        bpy.utils.register_class(cls)

    # メニューに項目追加
    bpy.types.TOPBAR_MT_editor_menus.append(TOPBAR_MT_my_menu.submenu)
    # 3Dビューに描画関数を追加
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(DrawCollider.draw_collider, (), "WINDOW", "POST_VIEW")

    print("レベルエディタが有効化されました")

#################################################################################

def unregister():
    # メニューから項目を削除
    bpy.types.TOPBAR_MT_editor_menus.remove(TOPBAR_MT_my_menu.submenu)

    # 3Dビューから描画関数を削除
    bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")

    # blenderからクラスを削除
    for cls in classes:
        bpy.utils.unregister_class(cls)

    print("レベルエディタが無効化されました")

#################################################################################

def rename_last_object_if_known_type():
    obj = bpy.context.active_object
    if obj is None or obj.type != 'MESH':
        print("対象オブジェクトが無効です")
        return

    mesh_name = obj.data.name.lower()
    print(f"rename対象: {obj.name}, data name: {mesh_name}")

    if "cube" in mesh_name or "立方体" in mesh_name:
        obj.name = "Cube"
    elif "uvsphere" in mesh_name or "sphere" in mesh_name or "球" in mesh_name:
        obj.name = "Sphere"
    elif "icosphere" in mesh_name or "ico" in mesh_name or "ICO球" in mesh_name:
        obj.name = "IcoSphere"
    elif "plane" in mesh_name or "平面" in mesh_name:
        obj.name = "Plane"
    elif "cylinder" in mesh_name or "円柱" in mesh_name:
        obj.name = "Cylinder"
    elif "cone" in mesh_name or "円錐" in mesh_name:
        obj.name = "Cone"

#################################################################################

if __name__ == "__main__":
    register()
