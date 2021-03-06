/*
 * $Id: gui.h 1336 2014-12-08 09:29:59Z justin $
 * Copyright (C) 2009 Lucid Fusion Labs

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LFL_LFAPP_GUI_H__
#define __LFL_LFAPP_GUI_H__
namespace LFL {

DECLARE_bool(multitouch);
DECLARE_bool(draw_grid);
DECLARE_string(console_font);

struct GUI : public MouseController {
    Box box;
    Window *parent;
    DrawableBoxArray child_box;
    Toggler toggle_active;
    GUI(Window *W=0, const Box &B=Box()) : box(B), parent(W), toggle_active(&active)
    { if (parent) parent->mouse_gui.push_back(this); }
    virtual ~GUI() { if (parent) VectorEraseByValue(&parent->mouse_gui, this); }

    DrawableBoxArray *Reset() { Clear(); return &child_box; }
    void Clear() { child_box.Clear(); MouseController::Clear(); }
    void UpdateBox(const Box &b, int draw_box_ind, int input_box_ind) {
        if (draw_box_ind  >= 0) child_box.data[draw_box_ind ].box = b;
        if (input_box_ind >= 0) hit           [input_box_ind].box = b;
    }
    void UpdateBoxX(int x, int draw_box_ind, int input_box_ind) {
        if (draw_box_ind  >= 0) child_box.data[draw_box_ind ].box.x = x;
        if (input_box_ind >= 0) hit           [input_box_ind].box.x = x;
    }
    void UpdateBoxY(int y, int draw_box_ind, int input_box_ind) {
        if (draw_box_ind  >= 0) child_box.data[draw_box_ind ].box.y = y;
        if (input_box_ind >= 0) hit           [input_box_ind].box.y = y;
    }

    virtual point MousePosition() const { return screen->mouse - box.TopLeft(); }
    virtual void SetLayoutDirty() { child_box.Clear(); }
    virtual void Layout(const Box &b) { box=b; Layout(); }
    virtual void Layout() {}
    virtual void Draw() {
        if (child_box.data.empty()) Layout();
        child_box.Draw(box.TopLeft());
    }
    virtual void HandleTextMessage(const string &s) {}

    virtual bool ToggleActive() {
        bool ret = toggle_active.Toggle();
        active ? Activate() : Deactivate();
        return ret;
    }
    virtual void ToggleConsole() { if (!active) app->shell.console(vector<string>()); }
};

struct Widget {
    struct Interface {
        GUI *gui;
        vector<int> hitbox;
        int drawbox_ind=-1;
        bool del_hitbox=0;
        virtual ~Interface() { if (del_hitbox) DelHitBox(); }
        Interface(GUI *g) : gui(g) {}

        void AddClickBox(const Box  &b, const MouseController::Callback &cb) {                   hitbox.push_back(gui->AddClickBox(b, cb)); }
        void AddHoverBox(const Box  &b, const MouseController::Callback &cb) {                   hitbox.push_back(gui->AddHoverBox(b, cb)); }
        void AddDragBox (const Box  &b, const MouseController::Callback &cb) {                   hitbox.push_back(gui->AddDragBox (b, cb)); }
        void AddClickBox(const Box3 &t, const MouseController::Callback &cb) { for (auto &b : t) hitbox.push_back(gui->AddClickBox(b, cb)); }
        void AddHoverBox(const Box3 &t, const MouseController::Callback &cb) { for (auto &b : t) hitbox.push_back(gui->AddHoverBox(b, cb)); }
        void AddDragBox (const Box3 &t, const MouseController::Callback &cb) { for (auto &b : t) hitbox.push_back(gui->AddDragBox (b, cb)); }
        void DelHitBox() { for (auto i = hitbox.begin(), e = hitbox.end(); i != e; ++i) gui->hit.Erase(*i); hitbox.clear(); }
        MouseController::HitBox &GetHitBox(int i=0) const { return gui->hit[hitbox[i]]; }
        Box GetHitBoxBox(int i=0) const { return Box::Add(GetHitBox(i).box, gui->box.TopLeft()); }
        DrawableBox *GetDrawBox() const { return drawbox_ind >= 0 ? VectorGet(gui->child_box.data, drawbox_ind) : 0; }
    };
    struct Vector : public vector<Interface*> {
        virtual ~Vector() {}
    };
    struct Window : public Box, Interface {
        virtual ~Window() {}
        Window(GUI *Gui, Box window) : Box(window), Interface(Gui) {}
        Window(GUI *Gui, int X, int Y, int W, int H) : Interface(Gui) { x=X; y=Y; w=W; h=H; }
    };
    struct Button : public Interface {
        Box box;
        Font *font=0;
        string text;
        point textsize;
        Color *outline=0;
        Drawable *drawable=0;
        MouseController::Callback cb;
        bool init=0, hover=0;
        int decay=0;
        Button() : Interface(0) {}
        Button(GUI *G, Drawable *D, Font *F, const string &T, const MouseController::Callback &CB)
            : Interface(G), font(F), text(T), drawable(D), cb(CB), init(1) { if (F && T.size()) SetText(T); }

        void SetBox(const Box &b) { box=b; hitbox.clear(); AddClickBox(box, cb); init=0; }
        void SetText(const string &t) { text = t; Box w; font->Size(text, &w); textsize = w.Dimension(); }
        void EnableHover() { AddHoverBox(box, MouseController::CB(bind(&Button::ToggleHover, this))); }
        void ToggleHover() { hover = !hover; }

        void Layout(Flow *flow, const point &d) { box.SetDimension(d); Layout(flow); }
        void Layout(Flow *flow) {
            flow->SetFont(0);
            flow->SetFGColor(&Color::white);
            LayoutComplete(flow, flow->out->data[flow->AppendBox(box.w, box.h, drawable)].box);
        }
        void LayoutBox(Flow *flow, const Box &b) {
            flow->SetFont(0);
            flow->SetFGColor(&Color::white);
            if (drawable) flow->out->PushBack(b, flow->cur_attr, drawable, &drawbox_ind);
            LayoutComplete(flow, b);
        }
        void LayoutComplete(Flow *flow, const Box &b) {
            SetBox(b);
            if (outline) {
                flow->SetFont(0);
                flow->SetFGColor(outline);
                flow->out->PushBack(box, flow->cur_attr, Singleton<BoxOutline>::Get());
            }
            point save_p = flow->p;
            flow->SetFont(font);
            flow->SetFGColor(0);
            flow->p = box.Position() + point(Box(0, 0, box.w, box.h).centerX(textsize.x), 0);
            flow->AppendText(text);
            flow->p = save_p;
        }
    };
    struct Scrollbar : public Interface {
        struct Flag { enum { Attached=1, Horizontal=2, AttachedHorizontal=Attached|Horizontal }; };
        Box win;
        int flag=0, doc_height=200, dot_size=25;
        float scrolled=0, last_scrolled=0, increment=20;
        Color color=Color(15, 15, 15, 55);
        Font *menuicon2=0;
        bool dragging=0, dirty=0;
        virtual ~Scrollbar() {}
        Scrollbar(GUI *Gui, Box window=Box(), int f=Flag::Attached) : Interface(Gui), win(window), flag(f),
        menuicon2(Fonts::Get("MenuAtlas2", "", 0, Color::black)) {
            if (win.w && win.h) { if (f & Flag::Attached) LayoutAttached(win); else LayoutFixed(win); } 
        }

        void LayoutFixed(const Box &w) { win = w; Layout(dot_size, dot_size, flag & Flag::Horizontal); }
        void LayoutAttached(const Box &w) {
            win = w;
            win.y = -win.h;
            int aw = dot_size, ah = dot_size;
            bool flip = flag & Flag::Horizontal;
            if (!flip) { win.x += win.w - aw; win.w = aw; }
            else win.h = ah;
            Layout(aw, ah, flip);
        }
        void Layout(int aw, int ah, bool flip) {
            Box arrow_down = win;
            if (flip) { arrow_down.w = aw; win.x += aw; }
            else      { arrow_down.h = ah; win.y += ah; }

            Box scroll_dot = arrow_down, arrow_up = win;
            if (flip) { arrow_up.w = aw; win.w -= 2*aw; arrow_up.x += win.w; }
            else      { arrow_up.h = ah; win.h -= 2*ah; arrow_up.y += win.h; }

            if (gui) {
                int attr_id = gui->child_box.attr.GetAttrId(Drawable::Attr());
                gui->child_box.PushBack(arrow_up,   attr_id, menuicon2 ? menuicon2->FindGlyph(flip ? 64 : 66) : 0);
                gui->child_box.PushBack(arrow_down, attr_id, menuicon2 ? menuicon2->FindGlyph(flip ? 65 : 61) : 0);
                gui->child_box.PushBack(scroll_dot, attr_id, menuicon2 ? menuicon2->FindGlyph(            72) : 0, &drawbox_ind);

                AddDragBox(scroll_dot, MouseController::CB(bind(&Scrollbar::DragScrollDot, this)));
                AddClickBox(arrow_up,   MouseController::CB(bind(flip ? &Scrollbar::ScrollDown : &Scrollbar::ScrollUp,   this)));
                AddClickBox(arrow_down, MouseController::CB(bind(flip ? &Scrollbar::ScrollUp   : &Scrollbar::ScrollDown, this)));
            }
            Update(true);
        }
        void Update(bool force=false) {
            if (!app->input.MouseButton1Down()) dragging = false;
            if (!dragging && !dirty && !force) return;
            bool flip = flag & Flag::Horizontal;
            int aw = dot_size, ah = dot_size;
            if (dragging) {
                if (flip) scrolled = Clamp(    (float)(gui->MousePosition().x - win.x) / win.w, 0, 1);
                else      scrolled = Clamp(1 - (float)(gui->MousePosition().y - win.y) / win.h, 0, 1);
            }
            if (flip) gui->UpdateBoxX(win.x          + (int)((win.w - aw) * scrolled), drawbox_ind, IndexOrDefault(hitbox, 0, -1));
            else      gui->UpdateBoxY(win.top() - ah - (int)((win.h - ah) * scrolled), drawbox_ind, IndexOrDefault(hitbox, 0, -1));
            dirty = false;
        }
        void SetDocHeight(int v) { doc_height = v; }
        void DragScrollDot() { dragging = true; dirty = true; }
        void ScrollUp  () { scrolled -= increment / doc_height; Clamp(&scrolled, 0, 1); dirty=true; }
        void ScrollDown() { scrolled += increment / doc_height; Clamp(&scrolled, 0, 1); dirty=true; }
        float ScrollDelta() { float ret=scrolled-last_scrolled; last_scrolled=scrolled; return ret; }
        float AddScrollDelta(float cur_val) { 
            scrolled = Clamp(cur_val + ScrollDelta(), 0, 1);
            if (EqualChanged(&last_scrolled, scrolled)) dirty = 1;
            return scrolled;
        }
    };
};

struct KeyboardGUI : public KeyboardController {
    typedef function<void(const string &text)> RunCB;
    Window *parent;
    Toggler toggle_active;
    Bind toggle_bind;
    RunCB runcb;
    RingVector<string> lastcmd;
    int lastcmd_ind=-1;
    KeyboardGUI(Window *W, Font *F, int LastCommands=50)
        : parent(W), toggle_active(&active), lastcmd(LastCommands) { parent->keyboard_gui.push_back(this); }
    virtual ~KeyboardGUI() { if (parent) VectorEraseByValue(&parent->keyboard_gui, this); }
    virtual void Enable() { active = true; }
    virtual bool Toggle() { return toggle_active.Toggle(); }
    virtual void Run(string cmd) { if (runcb) runcb(cmd); }
    virtual void SetToggleKey(int TK, int TM=Toggler::Default) { toggle_bind.key=TK; toggle_active.mode=TM; }

    void AddHistory  (const string &cmd);
    int  ReadHistory (const string &dir, const string &name);
    int  WriteHistory(const string &dir, const string &name, const string &hdr);
};

struct TextGUI : public KeyboardGUI {
    struct Lines;
    struct Link : public Widget::Interface {
        Box3 box;
        string link;
        TextGUI *parent=0;
        shared_ptr<Texture> image;
        Link(TextGUI *P, GUI *G, const Box3 &b, const string &U)
            : Interface(G), box(b), link(U), parent(P) {
            AddClickBox(b, MouseController::CB(bind(&Link::Visit, this)));
            AddHoverBox(b, MouseController::CoordCB(bind(&Link::Hover, this, _1, _2, _3, _4)));
            del_hitbox = true;
        }
        ~Link() { if (parent->hover_link == this) parent->hover_link = 0; }
        void Hover(int, int, int, int down) { parent->hover_link = down ? this : 0; }
        void Visit() { SystemBrowser::Open(link.c_str()); }
    };
    struct LineData {
        Box box;
        Flow flow;
        DrawableBoxArray glyphs;
        unordered_map<int, shared_ptr<Link> > links;
    };
    struct Line {
        point p;
        TextGUI *parent=0;
        TextGUI::Lines *cont=0;
        shared_ptr<LineData> data;
        Line() : data(new LineData()) {}
        Line &operator=(const Line &s) { data=s.data; return *this; }
        static void Move (Line &t, Line &s) { swap(t.data, s.data); }
        static void MoveP(Line &t, Line &s) { swap(t.data, s.data); t.p=s.p; }

        void Init(TextGUI *P, TextGUI::Lines *C) { parent=P; cont=C; data->flow=InitFlow(&data->glyphs); }
        Flow InitFlow(DrawableBoxArray *out) { return Flow(&data->box, parent->font, out, &parent->layout); }
        int GetAttrId(const Drawable::Attr &a) { return data->glyphs.attr.GetAttrId(a); }
        int Size () const { return data->glyphs.Size(); }
        int Lines() const { return 1+data->glyphs.line.size(); }
        string Text() const { return data->glyphs.Text(); }
        void Clear() { data->links.clear(); data->glyphs.Clear(); data->flow=InitFlow(&data->glyphs); }
        int Erase(int o, int l=INT_MAX);
        int AssignText(const StringPiece   &s, int a=0) { Clear(); return AppendText(s, a); }
        int AssignText(const String16Piece &s, int a=0) { Clear(); return AppendText(s, a); }
        int AppendText(const StringPiece   &s, int a=0) { return InsertTextAt(Size(), s, a); }
        int AppendText(const String16Piece &s, int a=0) { return InsertTextAt(Size(), s, a); }
        template <class X> int OverwriteTextAt(int o, const StringPieceT<X> &s, int a=0);
        template <class X> int InsertTextAt   (int o, const StringPieceT<X> &s, int a=0);
        template <class X> int UpdateText     (int o, const StringPieceT<X> &s, int a, int max_width=0, bool *append=0, int insert_mode=-1);
        int InsertTextAt(int o, const string   &s, int a=0) { return InsertTextAt<char> (o, s, a); }
        int InsertTextAt(int o, const String16 &s, int a=0) { return InsertTextAt<short>(o, s, a); }
        int UpdateText(int o, const string   &s, int attr, int max_width=0, bool *append=0) { return UpdateText<char> (o, s, attr, max_width, append); }
        int UpdateText(int o, const String16 &s, int attr, int max_width=0, bool *append=0) { return UpdateText<short>(o, s, attr, max_width, append); }
        void EncodeText(DrawableBoxArray *o, int x, const StringPiece   &s, int a=0) { Flow f=InitFlow(o); f.p.x=x; f.AppendText(s,a); }
        void EncodeText(DrawableBoxArray *o, int x, const String16Piece &s, int a=0) { Flow f=InitFlow(o); f.p.x=x; f.AppendText(s,a); }
        int Layout(int width=0, bool flush=0) { Layout(Box(0,0,width,0), flush); return Lines(); }
        void Layout(Box win, bool flush=0);
        point Draw(point pos, int relayout_width=-1, int g_offset=0, int g_len=-1);
    };
    struct LineTokenProcessor {
        Line *const L;
        bool sw=0, ew=0, pw=0, nw=0, overwrite=0, osw=1, oew=1;
        bool lbw=0, lew=0, nlbw=0, nlew=0;
        int x, line_size, erase, pi=0, ni=0;
        DrawableBoxRun v;
        LineTokenProcessor(Line *l, int o, const DrawableBoxRun &V, int Erase);
        void LoadV(const DrawableBoxRun &V) { FindBoundaryConditions((v=V), &sw, &ew); }
        void FindPrev(const DrawableBoxArray &g) { const Drawable *p; while (pi > 0           && (p = g[pi-1].drawable) && !isspace(p->Id())) pi--; }
        void FindNext(const DrawableBoxArray &g) { const Drawable *n; while (ni < line_size-1 && (n = g[ni+1].drawable) && !isspace(n->Id())) ni++; }
        void PrepareOverwrite(const DrawableBoxRun &V) { osw=sw; oew=ew; LoadV(V); erase=0; overwrite=1; }
        void ProcessUpdate();
        void ProcessResult();
        void SetNewLineBoundaryConditions(bool sw, bool ew) { nlbw=sw; nlew=ew; }
        static void FindBoundaryConditions(const DrawableBoxRun &v, bool *sw, bool *ew) {
            *sw = v.Size() && !isspace(v.First().Id());
            *ew = v.Size() && !isspace(v.Last ().Id());
        }
    };
    struct Lines : public RingVector<Line> {
        int wrapped_lines;
        function<void(Line&, Line&)> move_cb, movep_cb;
        Lines(TextGUI *P, int N) :
            RingVector<Line>(N), wrapped_lines(N),
            move_cb (bind(&Line::Move,  _1, _2)), 
            movep_cb(bind(&Line::MoveP, _1, _2)) { for (auto &i : data) i.Init(P, this); }

        Line *PushFront() { Line *l = RingVector<Line>::PushFront(); l->Clear(); return l; }
        Line *InsertAt(int dest_line, int lines=1, int dont_move_last=0) {
            CHECK(lines); CHECK_LT(dest_line, 0);
            int clear_dir = 1;
            if (dest_line == -1) { ring.PushBack(lines); clear_dir = -1; }
            else Move(*this, dest_line+lines, dest_line, -dest_line-lines-dont_move_last, move_cb);
            for (int i=0; i<lines; i++) (*this)[dest_line + i*clear_dir].Clear();
            return &(*this)[dest_line];
        }
        static int GetBackLineLines(const Lines &l, int i) { return l[-i-1].Lines(); }
    };
    struct LinesFrameBuffer : public RingFrameBuffer<Line> {
        typedef function<LinesFrameBuffer*(const Line*)> FromLineCB;
        struct Flag { enum { NoLayout=1, NoVWrap=2, Flush=4 }; };
        PaintCB paint_cb = &LinesFrameBuffer::PaintCB;
        int lines=0;

        LinesFrameBuffer *Attach(LinesFrameBuffer **last_fb);
        virtual bool SizeChanged(int W, int H, Font *font);
        virtual int Height() const { return lines * font_height; }
        tvirtual void Clear(Line *l) { RingFrameBuffer::Clear(l, Box(w, l->Lines() * font_height), true); }
        tvirtual void Update(Line *l, int flag=0);
        tvirtual void Update(Line *l, const point &p, int flag=0) { l->p=p; Update(l, flag); }
        tvirtual int PushFrontAndUpdate(Line *l, int xo=0, int wlo=0, int wll=0, int flag=0);
        tvirtual int PushBackAndUpdate (Line *l, int xo=0, int wlo=0, int wll=0, int flag=0);
        tvirtual void PushFrontAndUpdateOffset(Line *l, int lo);
        tvirtual void PushBackAndUpdateOffset (Line *l, int lo); 
        static point PaintCB(Line *l, point lp, const Box &b) { return Paint(l, lp, b); }
        static point Paint  (Line *l, point lp, const Box &b, int offset=0, int len=-1);
    };
    struct LinesGUI : public GUI {
        LinesFrameBuffer *fb;
        LinesGUI(Window *W=0, const Box &B=Box(), LinesFrameBuffer *FB=0) : GUI(W, B), fb(FB) {}
        bool NotActive() const { return !box.within(screen->mouse); }
        point MousePosition() const { return fb->BackPlus(screen->mouse - box.BottomLeft()); }
    };
    struct LineUpdate {
        enum { PushBack=1, PushFront=2, DontUpdate=4 }; 
        Line *v; LinesFrameBuffer *fb; int flag, o;
        LineUpdate(Line *V=0, LinesFrameBuffer *FB=0, int F=0, int O=0) : v(V), fb(FB), flag(F), o(O) {}
        LineUpdate(Line *V, const LinesFrameBuffer::FromLineCB &cb, int F=0, int O=0) : v(V), fb(cb(V)), flag(F), o(O) {}
        ~LineUpdate() {
            if (!fb->lines || (flag & DontUpdate)) v->Layout(fb->wrap ? fb->w : 0);
            else if (flag & PushFront) { if (o) fb->PushFrontAndUpdateOffset(v,o); else fb->PushFrontAndUpdate(v); }
            else if (flag & PushBack)  { if (o) fb->PushBackAndUpdateOffset (v,o); else fb->PushBackAndUpdate (v); }
            else fb->Update(v);
        }
        Line *operator->() const { return v; }
    };
    struct Cursor {
        enum { Underline=1, Block=2 };
        int type=Underline, attr=0;
        Time blink_time=Time(333), blink_begin=Time(0);
        point i, p;
    };
    struct Selection {
        bool enabled=1, changing=0, changing_previously=0;
        int gui_ind=-1;
        struct Point { 
            int line_ind=0, char_ind=0; point click; Box glyph;
            bool operator<(const Point &c) const { SortImpl2(c.glyph.y, glyph.y, glyph.x, c.glyph.x); }
            string DebugString() const { return StrCat("i=", click.DebugString(), " l=", line_ind, " c=", char_ind, " b=", glyph.DebugString()); }
        } beg, end;
        Box3 box;
    };

    LinesGUI mouse_gui;
    Font *font;
    Flow::Layout layout;
    Cursor cursor;
    Selection selection;
    Line cmd_line;
    LinesFrameBuffer cmd_fb;
    string cmd_prefix="> ";
    Color cmd_color=Color::white, selection_color=Color(Color::grey70, 0.5);
    bool deactivate_on_enter=0, token_processing=0, insert_mode=1;
    int start_line=0, end_line=0, start_line_adjust=0, skip_last_lines=0;
    function<void(const shared_ptr<Link>&)> new_link_cb;
    function<void(Link*)> hover_link_cb;
    Link *hover_link=0;

    TextGUI(Window *W, Font *F) : KeyboardGUI(W, F), mouse_gui(W), font(F)
    { layout.pad_wide_chars=1; cmd_line.Init(this,0); cmd_line.GetAttrId(Drawable::Attr(F)); }

    virtual ~TextGUI() {}
    virtual int CommandLines() const { return 0; }
    virtual void Input(char k) { cmd_line.UpdateText(cursor.i.x++, String16(1, k), cursor.attr); UpdateCommandFB(); UpdateCursor(); }
    virtual void Erase()       { if (!cursor.i.x) return;       cmd_line.Erase(--cursor.i.x, 1); UpdateCommandFB(); UpdateCursor(); }
    virtual void CursorRight() { cursor.i.x = min(cursor.i.x+1, cmd_line.Size()); UpdateCursor(); }
    virtual void CursorLeft()  { cursor.i.x = max(cursor.i.x-1, 0);               UpdateCursor(); }
    virtual void Home()        { cursor.i.x = 0;                                  UpdateCursor(); }
    virtual void End()         { cursor.i.x = cmd_line.Size();                    UpdateCursor(); }
    virtual void HistUp()      { if (int c=lastcmd.ring.count) { AssignInput(lastcmd[lastcmd_ind]); lastcmd_ind=max(lastcmd_ind-1, -c); } }
    virtual void HistDown()    { if (int c=lastcmd.ring.count) { AssignInput(lastcmd[lastcmd_ind]); lastcmd_ind=min(lastcmd_ind+1, -1); } }
    virtual void Enter();

    virtual string Text() const { return cmd_line.Text(); }
    virtual void AssignInput(const string &text)
    { cmd_line.AssignText(text); cursor.i.x=cmd_line.Size(); UpdateCommandFB(); UpdateCursor(); }

    virtual LinesFrameBuffer *GetFrameBuffer() { return &cmd_fb; }
    virtual void UpdateCursor() { cursor.p = cmd_line.data->glyphs.Position(cursor.i.x); }
    virtual void UpdateCommandFB() { 
        cmd_fb.fb.Attach();
        ScopedDrawMode drawmode(DrawMode::_2D);
        cmd_fb.PushBackAndUpdate(&cmd_line); // cmd_fb.OverwriteUpdate(&cmd_line, cursor.x)
        cmd_fb.fb.Release();
    }
    virtual void Draw(const Box &b);
    virtual void DrawCursor(point p);
    virtual void UpdateToken(Line*, int word_offset, int word_len, int update_type, const LineTokenProcessor*);
    virtual void UpdateLongToken(Line *BL, int beg_offset, Line *EL, int end_offset, const string &text, int update_type);
};

struct TextArea : public TextGUI {
    Lines line;
    LinesFrameBuffer line_fb;
    Time write_last=Time(0);
    const Border *clip=0;
    bool wrap_lines=1, write_timestamp=0, write_newline=1, reverse_line_fb=0;
    int line_left=0, end_line_adjust=0, start_line_cutoff=0, end_line_cutoff=0;
    int scroll_inc=10, scrolled_lines=0;
    float v_scrolled=0, h_scrolled=0, last_v_scrolled=0, last_h_scrolled=0;

    TextArea(Window *W, Font *F, int S=200) : TextGUI(W, F), line(this, S)
    { mouse_gui.fb = &line_fb; if (selection.enabled) InitSelection(); }
    virtual ~TextArea() {}

    /// Write() is thread-safe.
    virtual void Write(const string &s, bool update_fb=true, bool release_fb=true);
    virtual void PageUp  () { v_scrolled = Clamp(v_scrolled - (float)scroll_inc/(WrappedLines()-1), 0, 1); UpdateScrolled(); }
    virtual void PageDown() { v_scrolled = Clamp(v_scrolled + (float)scroll_inc/(WrappedLines()-1), 0, 1); UpdateScrolled(); }
    virtual void Resized(const Box &b);

    virtual void Redraw(bool attach=true);
    virtual void UpdateScrolled();
    virtual void UpdateHScrolled(int x, bool update_fb=true);
    virtual void UpdateVScrolled(int dist, bool reverse, int first_ind, int first_offset, int first_len);
    virtual int UpdateLines(float v_scrolled, int *first_ind, int *first_offset, int *first_len);
    virtual int WrappedLines() const { return line.wrapped_lines; }
    virtual LinesFrameBuffer *GetFrameBuffer() { return &line_fb; }

    virtual void Draw(const Box &w, bool cursor);
    virtual bool GetGlyphFromCoords(const point &p, Selection::Point *out) { return GetGlyphFromCoordsOffset(p, out, start_line, start_line_adjust); }
    bool GetGlyphFromCoordsOffset(const point &p, Selection::Point *out, int sl, int sla);

    bool Wrap() const { return line_fb.wrap; }
    int LineFBPushBack () const { return reverse_line_fb ? LineUpdate::PushFront : LineUpdate::PushBack;  }
    int LineFBPushFront() const { return reverse_line_fb ? LineUpdate::PushBack  : LineUpdate::PushFront; }
    int LayoutBackLine(Lines *l, int i) { return Wrap() ? (*l)[-i-1].Layout(line_fb.w) : 1; }

    void InitSelection();
    void DrawSelection();
    void DragCB(int button, int x, int y, int down);
    void CopyText(const Selection::Point &beg, const Selection::Point &end);
    string CopyText(int beg_line_ind, int beg_char_ind, int end_line_end, int end_char_ind, bool add_nl);
};

struct Editor : public TextArea {
    struct LineOffset { 
        long long offset; int size, wrapped_lines;
        LineOffset(int O=0, int S=0, int WL=1) : offset(O), size(S), wrapped_lines(WL) {}
        static string GetString(const LineOffset *v) { return StrCat(v->offset); }
        static int    GetLines (const LineOffset *v) { return v->wrapped_lines; }
        static int VectorGetLines(const vector<LineOffset> &v, int i) { return v[i].wrapped_lines; }
    };
    typedef PrefixSumKeyedRedBlackTree<int, LineOffset> LineMap;

    shared_ptr<File> file;
    LineMap file_line;
    FreeListVector<string> edits;
    int last_fb_width=0, last_fb_lines=0, last_first_line=0, wrapped_lines=0, fb_wrapped_lines=0;

    Editor(Window *W, Font *F, File *I, bool Wrap=0) : TextArea(W, F), file(I) {
        reverse_line_fb = 1;
        line_fb.wrap = Wrap;
        file_line.node_value_cb = &LineOffset::GetLines;
        file_line.node_print_cb = &LineOffset::GetString;
    }

    int WrappedLines() const { return wrapped_lines; }
    void UpdateWrappedLines(int cur_font_size, int width);
    int UpdateLines(float v_scrolled, int *first_ind, int *first_offset, int *first_len);
};

struct Terminal : public TextArea, public Drawable::AttrSource {
    struct State { enum { TEXT=0, ESC=1, CSI=2, OSC=3, CHARSET=4 }; };
    struct Attr {
        enum { Bold=1<<8, Underline=1<<9, Blink=1<<10, Reverse=1<<11, Italic=1<<12, Link=1<<13 };
        static void SetFGColorIndex(int *a, int c) { *a = (*a & ~0x0f) | ((c & 0xf)     ); }
        static void SetBGColorIndex(int *a, int c) { *a = (*a & ~0xf0) | ((c & 0xf) << 4); }
        static int GetFGColorIndex(int a) { return (a & 0xf) | ((a & Bold) ? (1<<3) : 0); }
        static int GetBGColorIndex(int a) { return (a>>4) & 0xf; }
    };
    struct Colors { Color c[16]; int normal_index, bold_index, bg_index; };
    struct StandardVGAColors : public Colors { StandardVGAColors(); };
    struct SolarizedColors : public Colors { SolarizedColors(); };

    int fd, term_width=0, term_height=0, parse_state=State::TEXT, default_cursor_attr=0;
    int scroll_region_beg=0, scroll_region_end=0;
    string parse_text, parse_csi, parse_osc;
    unsigned char parse_charset=0;
    bool parse_osc_escape=0, cursor_enabled=1, first_resize=1;
    point term_cursor=point(1,1), saved_term_cursor=point(1,1);
    LinesFrameBuffer::FromLineCB fb_cb;
    LinesFrameBuffer *last_fb=0;
    Border clip_border;
    Colors *colors=0;
    Color *bg_color=0;

    Terminal(int FD, Window *W, Font *F);
    virtual ~Terminal() {}
    virtual void Resized(const Box &b);
    virtual void ResizedLeftoverRegion(int w, int h, bool update_fb=true);
    virtual void SetScrollRegion(int b, int e, bool release_fb=false);
    virtual void SetDimension(int w, int h);
    virtual void Draw(const Box &b, bool draw_cursor);
    virtual void Write(const string &s, bool update_fb=true, bool release_fb=true);
    virtual void Input(char k) {                       write(fd, &k, 1); }
    virtual void Erase      () { char k = 0x7f;        write(fd, &k, 1); }
    virtual void Enter      () { char k = '\r';        write(fd, &k, 1); }
    virtual void Tab        () { char k = '\t';        write(fd, &k, 1); }
    virtual void Escape     () { char k = 0x1b;        write(fd, &k, 1); }
    virtual void HistUp     () { char k[] = "\x1bOA";  write(fd,  k, 3); }
    virtual void HistDown   () { char k[] = "\x1bOB";  write(fd,  k, 3); }
    virtual void CursorRight() { char k[] = "\x1bOC";  write(fd,  k, 3); }
    virtual void CursorLeft () { char k[] = "\x1bOD";  write(fd,  k, 3); }
    virtual void PageUp     () { char k[] = "\x1b[5~"; write(fd,  k, 4); }
    virtual void PageDown   () { char k[] = "\x1b[6~"; write(fd,  k, 4); }
    virtual void Home       () { char k = 'A' - 0x40;  write(fd, &k, 1); }
    virtual void End        () { char k = 'E' - 0x40;  write(fd, &k, 1);  }
    virtual void UpdateCursor() { cursor.p = point(GetCursorX(term_cursor.x, term_cursor.y), GetCursorY(term_cursor.y)); }
    virtual void UpdateToken(Line*, int word_offset, int word_len, int update_type, const LineTokenProcessor*);
    virtual int UpdateLines(float v_scrolled, int *first_ind, int *first_offset, int *first_len) { return 0; }
    virtual bool GetGlyphFromCoords(const point &p, Selection::Point *out) { return GetGlyphFromCoordsOffset(p, out, 0, 0); }
    virtual const Drawable::Attr *GetAttr(int attr) const;
    int GetCursorX(int x, int y) const {
        const Line *l = GetTermLine(y);
        return x <= l->Size() ? l->data->glyphs.Position(x-1).x : ((x-1) * font->FixedWidth());
    }
    int GetCursorY(int y) const { return (term_height - y + 1) * font->Height(); }
    int GetTermLineIndex(int y) const { return -term_height + y-1; }
    const Line *GetTermLine(int y) const { return &line[GetTermLineIndex(y)]; }
    /**/  Line *GetTermLine(int y)       { return &line[GetTermLineIndex(y)]; }
    Line *GetCursorLine() { return GetTermLine(term_cursor.y); }
    LinesFrameBuffer *GetPrimaryFrameBuffer()   { return line_fb.Attach(&last_fb); }
    LinesFrameBuffer *GetSecondaryFrameBuffer() { return cmd_fb .Attach(&last_fb); }
    LinesFrameBuffer *GetFrameBuffer(const Line *l) {
        int i = line.IndexOf(l);
        return ((-i-1 < start_line || term_height+i < skip_last_lines) ? cmd_fb : line_fb).Attach(&last_fb);
    }
    void SetColors(Colors *C) {
        colors = C;
        Attr::SetFGColorIndex(&default_cursor_attr, colors->normal_index);
        Attr::SetBGColorIndex(&default_cursor_attr, colors->bg_index);
        bg_color = &colors->c[colors->bg_index];
    }
    Border *UpdateClipBorder() {
        int font_height = font->Height();
        clip_border.top    = font_height * skip_last_lines;
        clip_border.bottom = font_height * start_line_adjust;
        return &clip_border;
    }
    void Scroll(int sy, int ey, int dy, bool move_fb_p) {
        CHECK_LT(sy, ey);
        int line_ind = GetTermLineIndex(sy), scroll_lines = ey - sy + 1, ady = abs(dy), sdy = (dy > 0 ? 1 : -1);
        Move(line, line_ind + (dy>0 ? dy : 0), line_ind + (dy<0 ? -dy : 0), scroll_lines - ady, move_fb_p ? line.movep_cb : line.move_cb);
        for (int i = 0, cy = (dy>0 ? sy : ey); i < ady; i++) GetTermLine(cy + i*sdy)->Clear();
    }
    void FlushParseText();
    void Newline(bool carriage_return=false);
    void NewTopline();
};

struct Console : public TextArea {
    string startcmd;
    double screen_percent=.4;
    Callback animating_cb;
    Time anim_time=Time(333), anim_begin=Time(0);
    bool animating=0, drawing=0, bottom_or_top=0, blend=1, ran_startcmd=0;
    Color color=Color(25,60,130,120);

    Console(Window *W, Font *F) : TextArea(W, F)
    { line_fb.wrap=write_timestamp=1; SetToggleKey(Key::Backquote); }

    virtual ~Console() {}
    virtual int CommandLines() const { return cmd_line.Lines(); }
    virtual void Run(string in) { app->shell.Run(in); }
    virtual void PageUp  () { TextArea::PageDown(); }
    virtual void PageDown() { TextArea::PageUp(); }
    virtual bool Toggle();
    virtual void Draw();
    int WriteHistory(const string &dir, const string &name)
    { return KeyboardGUI::WriteHistory(dir, name, startcmd); }
};

struct Dialog : public GUI {
    struct Flag { enum { None=0, Fullscreen=1, Next=2 }; };
    Font *font=0;
    Color color=Color(25,60,130,220);
    Box title, resize_left, resize_right, resize_bottom, close;
    bool deleted=0, moving=0, resizing_left=0, resizing_right=0, resizing_top=0, resizing_bottom=0, fullscreen=0;
    point mouse_start, win_start;
    int zsort=0;
    Dialog(float w, float h, int flag=0) : GUI(screen), font(Fonts::Get(FLAGS_default_font, "", 14, Color::white)) {
        screen->dialogs.push_back(this);
        box = screen->Box().center(screen->Box(w, h));
        fullscreen = flag & Flag::Fullscreen;
        Activate();
        Layout();
    }
    virtual ~Dialog() {}
    virtual void Draw();
    virtual void Layout() {
        Reset();
        if (fullscreen) return;
        title         = Box(0,       0,      box.w, screen->height*.05);
        resize_left   = Box(0,       -box.h, 3,     box.h);
        resize_right  = Box(box.w-3, -box.h, 3,     box.h);
        resize_bottom = Box(0,       -box.h, box.w, 3);

        Box close = Box(box.w-10, title.top()-10, 10, 10);
        AddClickBox(resize_left,   MouseController::CB(bind(&Dialog::Reshape,     this, &resizing_left)));
        AddClickBox(resize_right,  MouseController::CB(bind(&Dialog::Reshape,     this, &resizing_right)));
        AddClickBox(resize_bottom, MouseController::CB(bind(&Dialog::Reshape,     this, &resizing_bottom)));
        AddClickBox(title,         MouseController::CB(bind(&Dialog::Reshape,     this, &moving)));
        AddClickBox(close,         MouseController::CB(bind(&Dialog::MarkDeleted, this)));
    }
    void BringToFront() {
        if (screen->top_dialog == this) return;
        for (vector<Dialog*>::iterator i = screen->dialogs.begin(); i != screen->dialogs.end(); ++i) (*i)->zsort++; zsort = 0;
        sort(screen->dialogs.begin(), screen->dialogs.end(), LessThan);
        screen->top_dialog = this;
    }
    Box BoxAndTitle() const { return Box(box.x, box.y, box.w, box.h + title.h); }
    void Reshape(bool *down) { mouse_start = screen->mouse; win_start = point(box.x, box.y); *down = 1; }
    void MarkDeleted() { deleted = 1; }

    static bool LessThan(const Dialog *l, const Dialog *r) { return l->zsort < r->zsort; }
    static void MessageBox(const string &text);
    static void TextureBox(const string &text);
};

struct MessageBoxDialog : public Dialog {
    string message;
    Box messagesize;
    MessageBoxDialog(const string &m) : Dialog(.25, .1), message(m) { font->Size(message, &messagesize); }
    void Draw() {
        Dialog::Draw();
        { Scissor scissor(box); font->Draw(message, point(box.centerX(messagesize.w), box.centerY(messagesize.h)));  }
    }
};

struct TextureBoxDialog : public Dialog {
    Texture tex;
    TextureBoxDialog(const string &m) : Dialog(.25, .1) { tex.ID = ::atoi(m.c_str()); }
    void Draw() { Dialog::Draw(); tex.Draw(box); }
};

struct SliderTweakDialog : public Dialog {
    string flag_name;
    FlagMap *flag_map;
    Box flag_name_size;
    Widget::Scrollbar slider;
    SliderTweakDialog(const string &fn, float total=100, float inc=1) : Dialog(.3, .05),
    flag_name(fn), flag_map(Singleton<FlagMap>::Get()), slider(this, Box(), Widget::Scrollbar::Flag::Horizontal) {
        slider.increment = inc;
        slider.doc_height = total;
        slider.scrolled = atof(flag_map->Get(flag_name).c_str()) / total;
        font->Size(flag_name, &flag_name_size);
    }
    void Draw() { 
        Dialog::Draw();
        // slider.Draw(win);
        if (slider.dirty) {
            slider.dirty = 0;
            flag_map->Set(flag_name, StrCat(slider.scrolled * slider.doc_height));
        }
        font->Draw(flag_name, point(title.centerX(flag_name_size.w), title.centerY(flag_name_size.h)));
    }
};

struct EditorDialog : public Dialog {
    struct Flag { enum { Wrap=Dialog::Flag::Next }; };
    Editor editor;
    Box content_box;
    Widget::Scrollbar v_scrollbar, h_scrollbar;
    EditorDialog(Window *W, Font *F, File *I, float w=.5, float h=.5, int flag=0) :
        Dialog(w, h, flag), editor(W, F, I, flag & Flag::Wrap), v_scrollbar(this),
        h_scrollbar(this, Box(), Widget::Scrollbar::Flag::AttachedHorizontal) {}

    void Layout() {
        Dialog::Layout();
        content_box = box;
        if (1)              { v_scrollbar.LayoutAttached(box.Dimension()); content_box.w -= v_scrollbar.dot_size; }
        if (!editor.Wrap()) { h_scrollbar.LayoutAttached(box.Dimension()); MinusPlus(&content_box.h, &content_box.y, v_scrollbar.dot_size); }
    }
    void Draw() {
        bool wrap = editor.Wrap();
        if (1)     editor.active = screen->top_dialog == this;
        if (1)     editor.v_scrolled = v_scrollbar.AddScrollDelta(editor.v_scrolled);
        if (!wrap) editor.h_scrolled = h_scrollbar.AddScrollDelta(editor.h_scrolled);
        if (1)     editor.UpdateScrolled();
        if (1)     Dialog::Draw();
        if (1)     editor.Draw(content_box, true);
        if (1)     v_scrollbar.Update();
        if (!wrap) h_scrollbar.Update();
    }
};

namespace DOM {
struct Renderer : public Object {
    FloatContainer box;
    ComputedStyle style;
    bool style_dirty=1, layout_dirty=1;
    Flow *flow=0, *parent_flow=0, child_flow;
    DOM::Node *absolute_parent=0;
    shared_ptr<Texture> background_image;
    DrawableBoxArray child_box, child_bg;
    Tiles *tiles=0;

    Box content, padding, border, margin, clip_rect;
    Color color, background_color, border_top, border_bottom, border_right, border_left, outline;

    bool display_table_element=0, display_table=0, display_inline_table=0, display_block=0, display_inline=0;
    bool display_inline_block=0, display_list_item=0, display_none=0, block_level_box=0, establishes_block=0;
    bool position_relative=0, position_absolute=0, position_fixed=0, positioned=0, float_left=0, float_right=0, floating=0, normal_flow=0;
    bool done_positioned=0, done_floated=0, textalign_center=0, textalign_right=0, underline=0, overline=0, midline=0, blink=0, uppercase=0, lowercase=0, capitalize=0, valign_top=0, valign_mid=0;
    bool bgfixed=0, bgrepeat_x=0, bgrepeat_y=0, border_collapse=0, clear_left=0, clear_right=0, right_to_left=0, hidden=0, inline_block=0;
    bool width_percent=0, width_auto=0, height_auto=0, ml_auto=0, mr_auto=0, mt_auto=0, mb_auto=0, overflow_hidden=0, overflow_scroll=0, overflow_auto=0, clip=0, shrink=0, tile_context_opened=0;
    int width_px=0, height_px=0, ml_px=0, mr_px=0, mt_px=0, mb_px=0, bl_px=0, br_px=0, bt_px=0, bb_px=0, pl_px=0, pr_px=0, pt_px=0, pb_px=0, o_px=0;
    int lineheight_px=0, charspacing_px=0, wordspacing_px=0, valign_px=0, bgposition_x=0, bgposition_y=0, bs_t=0, bs_b=0, bs_r=0, bs_l=0, os=0;
    int clear_height=0, row_height=0, cell_colspan=0, cell_rowspan=0, extra_cell_height=0, max_child_i=-1;

    Renderer(Node *N) : style(N) {}

    void  UpdateStyle(Flow *F);
    Font* UpdateFont(Flow *F);
    void  UpdateDimensions(Flow *F);
    void  UpdateMarginWidth(Flow *F, int w);
    void  UpdateBackgroundImage(Flow *F);
    void  UpdateFlowAttributes(Flow *F);
    int ClampWidth(int w);
    void PushScissor(const Box &w);
    void Finish();

    bool Dirty() { return style_dirty || layout_dirty; }
    void InputActivate() { style.node->ownerDocument->gui->Activate(); }
    int TopBorderOffset    () { return pt_px + bt_px; }
    int RightBorderOffset  () { return pr_px + br_px; }
    int BottomBorderOffset () { return pb_px + bb_px; }
    int LeftBorderOffset   () { return pl_px + bl_px; }
    int TopMarginOffset    () { return mt_px + TopBorderOffset(); }
    int RightMarginOffset  () { return mr_px + RightBorderOffset(); }
    int BottomMarginOffset () { return mb_px + BottomBorderOffset(); }
    int LeftMarginOffset   () { return ml_px + LeftBorderOffset(); }
    point MarginPosition   () { return -point(LeftMarginOffset(), BottomMarginOffset()); }
    Border PaddingOffset   () { return Border(pt_px, pr_px, pb_px, pl_px); }
    Border BorderOffset    () { return Border(TopBorderOffset(), RightBorderOffset(), BottomBorderOffset(), LeftBorderOffset()); }
    Border MarginOffset    () { return Border(TopMarginOffset(), RightMarginOffset(), BottomMarginOffset(), LeftMarginOffset()); }
    int MarginWidth        () { return LeftMarginOffset() + RightMarginOffset(); }
    int MarginHeight       () { return TopMarginOffset () + BottomMarginOffset(); }
    int MarginBoxWidth     () { return width_auto ? 0 : (width_px + MarginWidth()); }
    int WidthAuto       (Flow *flow)        { return max(0, flow->container->w - MarginWidth()); }
    int MarginCenterAuto(Flow *flow, int w) { return max(0, flow->container->w - bl_px - pl_px - w - br_px - pr_px) / 2; }
    int MarginLeftAuto  (Flow *flow, int w) { return max(0, flow->container->w - bl_px - pl_px - w - RightMarginOffset()); } 
    int MarginRightAuto (Flow *flow, int w) { return max(0, flow->container->w - br_px - pr_px - w - LeftMarginOffset()); } 
}; }; // namespace DOM

struct Browser : public BrowserInterface {
    struct Document {
        DOM::BlockChainObjectAlloc alloc;
        DOM::HTMLDocument *node=0;
        string content_type, char_set;
        vector<StyleSheet*> style_sheet;
        DocumentParser *parser=0;
        JSContext *js_context=0;
        Console *js_console=0;
        int height=0;
        GUI gui;
        Widget::Scrollbar v_scrollbar, h_scrollbar;

        ~Document();
        Document(Window *W=0, const Box &V=Box());
        void Clear();
    };
    struct RenderLog { string data; int indent; };

    Layers layers;
    Document doc;
    RenderLog *render_log=0;
    Texture missing_image;
    point mouse, initial_displacement;
    Browser(Window *W=0, const Box &V=Box());

    Box Viewport() const { return doc.gui.box; }
    void Navigate(const string &url);
    void Open(const string &url);
    void KeyEvent(int key, bool down);
    void MouseMoved(int x, int y);
    void MouseButton(int b, bool d);
    void MouseWheel(int xs, int ys);
    void BackButton() {}
    void ForwardButton() {}
    void RefreshButton() {}
    void AnchorClicked(DOM::HTMLAnchorElement *anchor);
    void InitLayers() { layers.Init(2); }
    string GetURL() { return String::ToUTF8(doc.node->URL); }

    bool Dirty(Box *viewport);
    void Draw(Box *viewport);
    void Draw(Box *viewport, bool dirty);
    void Draw(Flow *flow, const point &displacement);
    void DrawScrollbar();

    void       DrawNode        (Flow *flow, DOM::Node*, const point &displacement);
    DOM::Node *LayoutNode      (Flow *flow, DOM::Node*, bool reflow);
    void       LayoutBackground(            DOM::Node*);
    void       LayoutTable     (Flow *flow, DOM::HTMLTableElement *n);
    void       UpdateTableStyle(Flow *flow, DOM::Node *n);
    void       UpdateRenderLog (            DOM::Node *n);

    static int ScreenToWebKitY(const Box &w) { return -w.y - w.h; }
};

BrowserInterface *CreateQTWebKitBrowser(Asset *a);
BrowserInterface *CreateBerkeliumBrowser(Asset *a, int w=1024, int h=1024);
BrowserInterface *CreateDefaultBrowser(Window *W, Asset *a, int w=1024, int h=1024);

struct HelperGUI : public GUI {
    HelperGUI(Window *W) : GUI(W), font(Fonts::Get(FLAGS_default_font, "", 9, Color::white)) {}
    Font *font;
    struct Hint { enum { UP, UPLEFT, UPRIGHT, DOWN, DOWNLEFT, DOWNRIGHT }; };
    struct Label {
        Box target, label; v2 target_center, label_center;
        int hint; string description;
        Label(const Box &w, const string &d, int h, Font *f, float ly, float lx) :
            target(w), target_center(target.center()), hint(h), description(d) {
            lx *= screen->width; ly *= screen->height;
            label_center = target_center;
            if      (h == Hint::UP   || h == Hint::UPLEFT   || h == Hint::UPRIGHT)   label_center.y += ly;
            else if (h == Hint::DOWN || h == Hint::DOWNLEFT || h == Hint::DOWNRIGHT) label_center.y -= ly;
            if      (h == Hint::UPRIGHT || h == Hint::DOWNRIGHT)                     label_center.x += lx;
            else if (h == Hint::UPLEFT  || h == Hint::DOWNLEFT)                      label_center.x -= lx;
            f->Size(description.c_str(), &label);
            AssignLabelBox();
        }
        void AssignLabelBox() { label.x = label_center.x - label.w/2; label.y = label_center.y - label.h/2; }
    };
    vector<Label> label;
    void AddLabel(const Box &w, const string &d, int h, float ly=.05, float lx=.02) { label.push_back(Label(w, d, h, font, ly, lx)); }
    void Activate() { active=1; /* ForceDirectedLayout(); */ }
    void ForceDirectedLayout();
    void Draw();
};

}; // namespace LFL
#endif // __LFL_LFAPP_GUI_H__
