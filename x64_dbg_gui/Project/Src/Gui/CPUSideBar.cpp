#include "CPUSideBar.h"
#include "Configuration.h"

CPUSideBar::CPUSideBar(CPUDisassembly *Ptr, QWidget *parent) : QAbstractScrollArea(parent)
{
    topVA = -1;
    selectedVA = -1;
    viewableRows = 0;

    CodePtr = Ptr;

    m_DefaultFont = CodePtr->font();
    this->setFont(m_DefaultFont);

    const QFontMetrics metrics(m_DefaultFont);
    fontWidth  = metrics.width(' ');
    fontHeight = metrics.height();

    InstrBuffer = CodePtr->instructionsBuffer();

    backgroundColor = ConfigColor("SideBarBackgroundColor");

    connect(Bridge::getBridge(), SIGNAL(disassembleAt(int_t, int_t)), this, SLOT(disassembleAt(int_t, int_t)));
}

QSize CPUSideBar::sizeHint() const
{
    return QSize(40,this->viewport()->height());
}

void CPUSideBar::disassembleAt(int_t parVA, int_t parCIP)
{
    MessageBeep(0);
}

void CPUSideBar::debugStateChangedSlot(DBGSTATE state)
{
    if(state==stopped)
    {
        repaint(); //clear
    }
}

void CPUSideBar::repaint()
{
    fontHeight = CodePtr->getRowHeight();
    viewport()->repaint();
}

void CPUSideBar::changeTopmostAddress(int i)
{
    if(i!=topVA)
    {
        topVA = i;
        //qDebug() << i;
        repaint();
    }
}

void CPUSideBar::setViewableRows(int rows)
{
    viewableRows = rows;
}

void CPUSideBar::setSelection(int_t selVA)
{
    if(selVA != selectedVA)
    {
        selectedVA = selVA;
        repaint();
    }
}

bool CPUSideBar::isJump(int i) const
{
    int BranchType=InstrBuffer->at(i).disasm.Instruction.BranchType;
    if(BranchType && BranchType!=RetType && BranchType!=CallType)
    {
        return true;
    }
    return false;
}

void CPUSideBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this->viewport());

    // Paints background
    painter.fillRect(painter.viewport(), QBrush(backgroundColor));

    if(InstrBuffer->size() == 0)
        return;

    int jumpoffset = 0;

    int_t last_va = InstrBuffer->last().rva + CodePtr->getBase();
    int_t first_va = InstrBuffer->first().rva + CodePtr->getBase();

    for(int line=0; line<viewableRows; line++)
    {
        Instruction_t instr = InstrBuffer->at(line);
        int_t instrVA = instr.rva + CodePtr->getBase();

        // draw bullet
        drawBullets(&painter, line, DbgGetBpxTypeAt(instrVA) == bp_none);

        if(isJump(line)) //handle jumps
        {
            bool isJumpGoingToExecute = DbgIsJumpGoingToExecute(instrVA);
            bool isSelected = (selectedVA == instrVA);

            /*
            if(CodePtr->currentEIP() != InstrBuffer->at(line).rva) //create a setting for this
                isJumpGoingToExecute=false;
            */

            jumpoffset++;

            int_t destVA = (int_t)instr.disasm.Instruction.AddrValue;

            if(instr.disasm.Instruction.Opcode == 0xFF)
                continue;

            bool isConditional = !((instr.disasm.Instruction.Opcode == 0xEB) || instr.disasm.Instruction.Opcode == 0xE9);

            if(destVA <= last_va && destVA >= first_va)
            {
                int destLine = line + 1;
                while(instrVA != destVA && destLine < viewableRows)
                    destLine++;
                drawJump(&painter, line, destLine, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
            }
            else if(destVA > last_va)
                drawJump(&painter, line, viewableRows+6, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
            else if(destVA < first_va)
                drawJump(&painter, line, -6, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
        }

        if(InstrBuffer->at(line).rva == CodePtr->currentEIP())
#ifdef _WIN64
            drawLabel(&painter, line, "RIP");
#else //x86
            drawLabel(&painter, line, "EIP");
#endif
    }
}

void CPUSideBar::drawJump(QPainter* painter, int startLine,int endLine,int jumpoffset, bool conditional, bool isexecute, bool isactive)
{
    painter->save();
    if(!conditional)
        painter->setPen(QPen(QColor("#000000"),1, Qt::SolidLine));  // jmp
    else
        painter->setPen(QPen(QColor("#000000"),1, Qt::DashLine));
    QPen tmp = painter->pen();

    if(isactive)
    {
        if(isexecute) //only highlight selected jumps
            tmp.setColor(Qt::red);
        tmp.setWidth(2);
    }
    painter->setPen(tmp);

    const int JumpPadding = 15;

    const int x = viewport()->width()-jumpoffset*JumpPadding - 12;
    const int x_right = viewport()->width()- 12;
    const int y_start =  fontHeight*(1+startLine)-0.5*fontHeight;
    const int y_end =  fontHeight*(1+endLine)-0.5*fontHeight;

    // vertical
    painter->drawLine(x,y_start,x,y_end);
    // start horizontal
    painter->drawLine(x,y_start,x_right,y_start);
    painter->drawLine(x,y_end,x_right,y_end);

    const int ArrowSizeX = 2;  // width  of arrow tip in pixel
    const int ArrowSizeY = 3;  // height of arrow tip in pixel


    tmp = painter->pen();
    tmp.setStyle(Qt::SolidLine);
    tmp.setWidth(2);
    painter->setPen(tmp);
    painter->drawLine(x_right-ArrowSizeX,y_end-ArrowSizeY,x_right,y_end);
    painter->drawLine(x_right-ArrowSizeX,y_end+ArrowSizeY,x_right,y_end);


    painter->restore();
}

void CPUSideBar::drawBullets(QPainter* painter, int line, bool isbp)
{
    painter->save();

    if( isbp)
        painter->setBrush(QBrush("#808080"));
    else
        painter->setBrush(QBrush(Qt::red));

    const int radius = fontHeight/2; //14/2=7
    const int y = line*fontHeight; //initial y
    const int yAdd = (fontHeight-radius)/2+1;
    const int x = viewport()->width() - 10; //initial x

    //painter->drawLine(0, y, viewport()->width(), y); //draw raster

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen("#ffffff"));
    painter->drawEllipse(x, y+yAdd, radius, radius);

    painter->restore();
}

void CPUSideBar::drawLabel(QPainter* painter, int Line, QString Text)
{
    painter->save();
    const int LineCoordinate = fontHeight*(1+Line);
    int length = Text.length();

    const QColor IPLabel = ConfigColor("SideBarIpLabelColor");
    const QColor IPLabelBG = ConfigColor("SideBarIpLabelBackgroundColor");

    int width = length*fontWidth + 2;
    int x = 1;
    int y = LineCoordinate-fontHeight;

    QRect rect(x, y, width, fontHeight);

    //draw rectangle
    painter->setBrush(IPLabelBG);
    painter->setPen(QPen(IPLabelBG));
    painter->drawRect(rect);

    //draw text inside the rectangle
    painter->setPen(QPen(IPLabel));
    painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, Text);

    //draw arrow
    y = fontHeight*(1+Line)-0.5*fontHeight;
    //y+=3;
    painter->setPen(QPen(IPLabelBG, 2));
    painter->setBrush(QBrush(IPLabelBG));
    drawStraightArrow(painter, rect.right()+2, y, this->viewport()->width()-x-15, y);

    painter->restore();
}

void CPUSideBar::drawStraightArrow(QPainter *painter, int x1, int y1, int x2, int y2)
{
    painter->drawLine(x1,y1,x2,y2);

    /*
    // this does not work
    QPainterPath path;

    path.moveTo( QPointF(x2, x2));
    path.lineTo (QPointF(x2-ArrowSizeX,y2-ArrowSizeY));
    path.lineTo (QPointF(x2-ArrowSizeX,y2+ArrowSizeY));
    path.closeSubpath();

    painter->setPen (Qt :: NoPen);
    painter->fillPath (path, QBrush (color));*/
    const int ArrowSizeX = 4;  // width  of arrow tip in pixel
    const int ArrowSizeY = 4;  // height of arrow tip in pixel

    painter->drawLine(x2,y2,x2-ArrowSizeX,y2-ArrowSizeY);
    painter->drawLine(x2,y2,x2-ArrowSizeX,y2+ArrowSizeY);

}
