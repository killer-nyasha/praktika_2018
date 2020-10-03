using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Runtime::InteropServices;
using namespace System::IO;

double function(double x)
{
	if (x > 9)
		return Math::Log(Math::Cos(x)) + Math::Log(Math::Sin(x)) + 2.0*Math::Exp(x) - 3.7;
	else if (x > 0)
		return 2 * x * x;
	else return x - 1;
}

//==============================================================================
//Для хранения функций

typedef double (*doublefunction)(double);

value struct Operation
{
	double scaleX;
	double scaleY;
	double translationX;
	double translationY;

	Operation(int k)
	{
		scaleX = 1;
		scaleY = 1;
		translationX = 0;
		translationY = 0;
	}
};

value struct PointsArrays
{
	List<double>^ x;
	List<double>^ y;

	PointsArrays(int i)
	{
		x = gcnew List<double>(i);
		y = gcnew List<double>(i);
	}
};

ref class Function
{
public:

	virtual PointsArrays getPoints(double min, double max, double delta) abstract;

	String^ name;
	Pen^ pen;

	Operation op = Operation(0);

	static array<Pen^>^ pens = gcnew array<Pen^>(5) 
	{ Pens::Red, Pens::Blue, Pens::Green, Pens::Magenta, Pens::Cyan };
	static int index = 0;
	static int pensCount = 5;

	Function()
	{
		pen = pens[index++ % pensCount];
	}

	virtual String^ ToString() override
	{
		return name;
	}
};

ref class TableFunction : Function
{
public:

	PointsArrays values = PointsArrays(50);

	void load(String^ path)
	{
		name = path->Substring(path->LastIndexOf('\\') + 1);
		name = name->Remove(name->IndexOf('.'));

		int k = 0;
		StreamReader^ sr = gcnew StreamReader(path);
		while (!sr->EndOfStream)
		{
			array<String^>^ arr; 
			arr = sr->ReadLine()->Split(' ');
			for (int i = 0; i < arr->Length; i++)
			{
				arr[i] = arr[i]->Trim();
				if (arr[i]->Length == 0) continue;
				if (k % 2 == 0)
					values.x->Add(double::Parse(arr[i]));//[k++ / 2]
				else values.y->Add(double::Parse(arr[i]));//[k++ / 2]
				k++;
			}
		}
		sr->Close();
	}

	PointsArrays getPoints(double min, double max, double delta) override
	{
		//ДОРАБОТАТЬ!!!
		return values;
	}
};

ref class PointerFunction : Function
{
public:
	doublefunction function;

	void load(doublefunction f)
	{
		name = "default";
		function = f;
	}

	PointsArrays getPoints(double min, double max, double delta) override
	{
		PointsArrays p = PointsArrays((max - min) / delta + 10);
		for (double x = min; x <= max; x += delta)
		{
			p.x->Add(x); p.y->Add(function(x));
		}
		return p;
	}
};

//==============================================================================
//Главное окно программы

ref class myForm : Form
{
public:

	//==================================================================
	//Обработка колёсика мыши

	void OnMouseWheel(Object ^sender, MouseEventArgs ^e)
	{
		scale *= 1 - e->Delta/500.0;
		if (scale < minscale)
			scale = minscale;
		if (scale > maxscale)
			scale = maxscale;
		Refresh();
	}

	//==================================================================
	//Обработка движений мыши

	bool mouseDown = false;
	Point lastLoc;
	Point mouse;
	DateTime dt;

	void OnMouseDown(Object ^sender, MouseEventArgs ^e)
	{
		if (!mouseDown)
		{
			mouseDown = true;
			lastLoc = e->Location;
			dt = DateTime::Now;
		}
	}

	void OnMouseUp(Object ^sender, MouseEventArgs ^e)
	{
		mouseDown = false;
		moveCamera();
		lastLoc = mouse;
	}

	void moveCamera()
	{
		center = Point(
			center.X + mouse.X - lastLoc.X,
			center.Y + mouse.Y - lastLoc.Y);
		Refresh();
	}

	void OnMouseMove(Object ^sender, MouseEventArgs ^e)
	{
		mouse = e->Location;

		if (mouseDown && (DateTime::Now - dt).TotalMilliseconds > 350)
		{
			moveCamera();
			lastLoc = e->Location;
			dt = DateTime::Now;
		}
	}

	//==================================================================
	//Расширение возможностей ListBox

	void DrawItemHandler(Object^ sender, DrawItemEventArgs^ e)
	{
		if (e->Index == -1) return;

		if ((e->State & DrawItemState::Selected) == DrawItemState::Selected)
			e = gcnew DrawItemEventArgs(e->Graphics,
				e->Font,
				e->Bounds,
				e->Index,
				e->State & ~DrawItemState::Selected,
				e->ForeColor, 
				Color::FromArgb(255, 255, 255));

		e->DrawBackground();
		e->DrawFocusRectangle(); 

		((SolidBrush^)textBrush)->Color = ((Function^)(functionsList->Items[e->Index]))->pen->Color;

		e->Graphics->DrawString(functionsList->Items[e->Index]->ToString(),
			ffont,
			textBrush,
			e->Bounds);
	}

	void MeasureItemHandler(Object^ sender, MeasureItemEventArgs^ e)
	{
		e->ItemHeight = 32;
	}

	//==================================================================
	//Конструктор и поля формы

	void addTools(array<String^>^ arr)
	{
		for (int i = 0; i < arr->Length; i++)
		{
			ToolBarButton^ toolBarButton1 = gcnew ToolBarButton();
			toolBarButton1->Text = arr[i];
			mainMenu->Buttons->Add(toolBarButton1);
		}
	}

	Color getDarkerColor(Color c)
	{
		return Color::FromArgb(c.R * 7 / 8, c.G * 7 / 8, c.B * 7 / 8);
	}

	myForm()
	{
		MouseWheel += gcnew MouseEventHandler(this, &myForm::OnMouseWheel);
		MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &myForm::OnMouseDown);
		MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &myForm::OnMouseUp);
		MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &myForm::OnMouseMove);

		this->DoubleBuffered = true;

		leftMenuTextBrush = gcnew SolidBrush(Color::Black);
		font = gcnew System::Drawing::Font("Times New Roman", 11, FontStyle::Bold);
		textBrush = gcnew SolidBrush(Color::FromArgb(150, 0, 0));

		mainMenu = gcnew ToolBar();

		this->BackColor = Color::FromArgb(250, 240, 200);
		this->Size = System::Drawing::Size(625, 625);
		
		Controls->Add(mainMenu);

		addTools(
			gcnew array<String^>(3)
		{ "Open", "Delete", "Color"});

		mainMenu->ButtonClick += gcnew System::Windows::Forms::ToolBarButtonClickEventHandler(this, &myForm::ToolBarButtonClick);

		ofd = gcnew OpenFileDialog();
		ofd->InitialDirectory = "C:\\";
		ofd->Filter = "txt files (*.txt)|*.txt|All files (*.*)|*.*";
		ofd->FilterIndex = 2;
		ofd->RestoreDirectory = true;
		ofd->ShowHelp = true;

		functionsList = gcnew ListBox();
		functionsList->Location = Point(0, mainMenu->Size.Height);
		functionsList->Size = System::Drawing::Size(75, 250);
		functionsList->Dock = DockStyle::Left;

		colorDialog = gcnew ColorDialog();
		coordPen = Pens::Black;
		
		label1 = gcnew Label();
		label1->Location = Point(75, 50);
		label1->Text = "translation X";
		Controls->Add(label1);
		label2 = gcnew Label();
		label2->Location = Point(75, 75-4);
		label2->Text = "translation Y";
		Controls->Add(label2);
		label3 = gcnew Label();
		label3->Location = Point(75, 100-8);
		label3->Text = "scale X";
		Controls->Add(label3);
		label4 = gcnew Label();
		label4->Location = Point(75, 125-12);
		label4->Text = "scale Y";
		Controls->Add(label4);

		numRight = gcnew NumericUpDown();
		numRight->MinimumSize = System::Drawing::Size(0,0);
		numRight->Width = 50;
		numRight->Location = Point(175, 50);

		numUp = gcnew NumericUpDown();
		numUp->MinimumSize = System::Drawing::Size(0, 0);
		numUp->Width = 50;
		numUp->Location = Point(175, 75 - 4);

		numRX = gcnew NumericUpDown();
		numRX->MinimumSize = System::Drawing::Size(0, 0);
		numRX->Width = 50;
		numRX->Location = Point(175, 100 - 8);

		numRY = gcnew NumericUpDown();
		numRY->MinimumSize = System::Drawing::Size(0, 0);
		numRY->Width = 50;
		numRY->Location = Point(175, 125 - 12);

		numRX->Value = 100;
		numRY->Value = 100;

		numRight->ValueChanged += gcnew System::EventHandler(this, &myForm::OnRight);
		numUp->ValueChanged += gcnew System::EventHandler(this, &myForm::OnUp);
		numRX->ValueChanged += gcnew System::EventHandler(this, &myForm::OnRX);
		numRY->ValueChanged += gcnew System::EventHandler(this, &myForm::OnRY);

		numRX->Minimum = -1000;
		numRX->Maximum = 1000;
		numRY->Minimum = -1000;
		numRY->Maximum = 1000;

		numRX->Increment = 10;
		numRY->Increment = 10;

		numRight->Minimum = -1000;
		numRight->Maximum = 1000;
		numUp->Minimum = -1000;
		numUp->Maximum = 1000;

		Controls->Add(numRight);
		Controls->Add(numUp);
		Controls->Add(numRX);
		Controls->Add(numRY);

		PointerFunction^ f = gcnew PointerFunction();
		f->load(function);
		functionsList->Items->Add(f);

		functionsList->DrawMode = DrawMode::OwnerDrawVariable;
		functionsList->DrawItem += gcnew DrawItemEventHandler(this, &myForm::DrawItemHandler);
		functionsList->MeasureItem += gcnew System::Windows::Forms::MeasureItemEventHandler(this, &myForm::MeasureItemHandler);
		functionsList->SelectedIndexChanged += gcnew System::EventHandler(this, &myForm::OnSelectedIndexChanged);
		functionsList->BackColor = getDarkerColor(this->BackColor);//Color::FromArgb(220, 240, 180);

		Controls->Add(functionsList);

		OnSelectedIndexChanged(nullptr, nullptr);
	}


	ToolBar^ mainMenu;
	ListBox^ functionsList;
	Label^ label1, ^label2, ^label3, ^label4;
	NumericUpDown^ numRight;
	NumericUpDown^ numUp;
	NumericUpDown^ numRX;
	NumericUpDown^ numRY;

	SolidBrush^ textBrush;
	SolidBrush^ leftMenuTextBrush;
	Pen^ coordPen;

	System::Drawing::Font^ ffont = gcnew System::Drawing::Font("Times New Roman",
		10, FontStyle::Regular);
	System::Drawing::Font^ font;

	OpenFileDialog^ ofd;
	ColorDialog^ colorDialog;

	int sz = 600;
	Point size = Point(sz / 2, sz / 2);
	Point center = Point(sz / 2, sz / 2);

	double scale = 0.025;
	double minscale = 0.005; 
	double maxscale = 1000;

							//scale = 0.01 - это
							//1 ед. графика = 100 пикселей
							//1 пиксель = 0.01 ед графика

	static const int offset = 8;

	//==================================================================
	//Преобразование коордиант графика в экранные коордианты

	double graphToScreenY(double y)
	{
		return center.Y - y / scale;
	}
	double graphToScreenX(double x)
	{
		return center.X + x / scale;
	}
	Point PointFromGraph(double x, double y)
	{
		return Point(graphToScreenX(x), graphToScreenY(y));
	}

	//Подбираем интервалы
	double selectScaleInterval(double max, int maxScaleDiv)
	{
		double scaleInterval = 1;
		int i = 1;

		if (max / scaleInterval > maxScaleDiv)
			while (max / scaleInterval > maxScaleDiv + 1)
				scaleInterval *= (i++ % 3 != 2 ? 2 : 2.5);
		else if (max / scaleInterval < maxScaleDiv)
			while (max / scaleInterval < maxScaleDiv - 1)
				scaleInterval /= (i++ % 3 != 2 ? 2 : 2.5);

		return scaleInterval;
	}

	void OnPaint(PaintEventArgs^ e) override
	{
		e->Graphics->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		//Оси координат
		e->Graphics->DrawLine(coordPen, 
			Point(center.X - size.X, center.Y), 
			Point(center.X + size.X, center.Y));
		e->Graphics->DrawLine(coordPen,
			Point(center.X, center.Y - size.Y),
			Point(center.X, center.Y + size.Y));

		//Стрелка на оси Y
		e->Graphics->DrawLine(coordPen,
			Point(center.X - 7, center.Y - size.Y + 15),
			Point(center.X, center.Y - size.Y));
		e->Graphics->DrawLine(coordPen,
			Point(center.X, center.Y - size.Y),
			Point(center.X + 7, center.Y - size.Y + 15)); 

		//Стрелка на оси X
		e->Graphics->DrawLine(coordPen,
			Point(center.X + size.X - 15, center.Y - 7),
			Point(center.X + size.X, center.Y));
		e->Graphics->DrawLine(coordPen,
			Point(center.X + size.X - 15, center.Y + 7),
			Point(center.X + size.X, center.Y));


		//Определяем границы графика
		double maxX = size.X * scale;
		double maxY = size.X * scale;

		int maxScaleDiv = 5;//максимальное количество делений на шкале
		double scaleXWidth = graphToScreenX(size.X);//ширина шкалы в пикселях
		
		double scaleIntervalX = selectScaleInterval(maxX, maxScaleDiv);//интервал одного деления
		double scaleIntervalY = selectScaleInterval(maxY, maxScaleDiv);

		//рисуем шкалу
		for (int j = -1; j <= 1; j += 2)
		for (double i = 0; Math::Abs(i) <= maxY; i += scaleIntervalY*j)
		{
			e->Graphics->DrawLine(coordPen, Point(center.X - 2, graphToScreenY(i)), Point(center.X + 2, graphToScreenY(i)));
			String^ s = i.ToString();
			System::Drawing::Size textsz = TextRenderer::MeasureText(s, font);
			if (i != 0)
				e->Graphics->DrawString(s, font, leftMenuTextBrush, 
					Point(center.X + 2 - textsz.Width,
						graphToScreenY(i) - textsz.Height / 2));
		}
		for (int j = -1; j <= 1; j += 2)
		for (double i = 0; Math::Abs(i) <= maxX; i += scaleIntervalX*j)
		{
			e->Graphics->DrawLine(coordPen, Point(graphToScreenX(i), center.Y - 2), Point(graphToScreenX(i), center.Y + 2));
			String^ s = i.ToString();
			System::Drawing::Size textsz = TextRenderer::MeasureText(s, font);
			if (i != 0)
				e->Graphics->DrawString(i.ToString(), font, leftMenuTextBrush, 
					Point(graphToScreenX(i) - textsz.Width / 2,
						center.Y + 2 - textsz.Height / 2 + offset));
		}

		double delta = scale*4;
		//интерполяция берётся так, что 
		//максимальная погрешность - 4 пикселя

		for (int i = 0; i < functionsList->Items->Count; i++)
		{
			PointsArrays p = ((Function^)functionsList->Items[i])->getPoints(Math::Max(-5, (int)-maxX), Math::Min(5, (int)maxX), delta);
			RenderGraphic(e, p, ((Function^)functionsList->Items[i])->pen, ((Function^)functionsList->Items[i])->op);
		}
	}

	void RenderGraphic(PaintEventArgs^ e, PointsArrays p, Pen^ pen, Operation op)
	{
		double fprev = p.y[0];
		for (int i = 1; i < p.x->Count; i++)
		{
			double fnew = p.y[i];
			e->Graphics->DrawLine(
				pen,
				PointFromGraph(p.x[i - 1] * op.scaleX + op.translationX, fprev * op.scaleY + op.translationY),
				PointFromGraph(p.x[i] * op.scaleX + op.translationX, fnew * op.scaleY + op.translationY));
			fprev = fnew;
		}
	}
	
	void ToolBarButtonClick(Object ^sender, ToolBarButtonClickEventArgs ^e)
	{
		if (e->Button->Text == "Open")
		{
			System::Windows::Forms::DialogResult res = ofd->ShowDialog();
			if (res == System::Windows::Forms::DialogResult::OK)
			{
				TableFunction^ tf1 = gcnew TableFunction();
				tf1->load(ofd->FileName);
				functionsList->Items->Add(tf1);
				Refresh();
			}
		}
		else if (e->Button->Text == "Delete")
		{
			if (functionsList->SelectedIndex != -1)
				functionsList->Items->RemoveAt(functionsList->SelectedIndex);
			Refresh();
		}
		else if (e->Button->Text == "Color")
		{
			int index = functionsList->SelectedIndex;
			System::Windows::Forms::DialogResult res = colorDialog->ShowDialog();
			if (res == System::Windows::Forms::DialogResult::OK)
			{
				if (index != -1)
				((Function^)functionsList->Items[index])->pen = gcnew Pen(colorDialog->Color);
				else 
				{
					BackColor = colorDialog->Color; 
					functionsList->BackColor = getDarkerColor(colorDialog->Color);

					if (colorDialog->Color.R + colorDialog->Color.G + colorDialog->Color.B > 64*3)
					{
						leftMenuTextBrush->Color = Color::Black;
						coordPen = Pens::Black;

						label1->ForeColor = Color::Black;
						label2->ForeColor = Color::Black; 
						label3->ForeColor = Color::Black; 
						label4->ForeColor = Color::Black;
							

					}
					else
					{
						leftMenuTextBrush->Color = Color::White;
						coordPen = Pens::White;

						label1->ForeColor = Color::White;
						label2->ForeColor = Color::White;
						label3->ForeColor = Color::White;
						label4->ForeColor = Color::White;
					}
				}
				Refresh();
			}
		}
	}

	void OnRight(System::Object ^sender, System::EventArgs ^e)
	{
		int index = functionsList->SelectedIndex;
		if (index == -1) return;

		((Function^)functionsList->Items[index])->op.translationX = (double)numRight->Value;
		Refresh();
	}

	void OnUp(System::Object ^sender, System::EventArgs ^e)
	{
		int index = functionsList->SelectedIndex;
		if (index == -1) return;

		((Function^)functionsList->Items[index])->op.translationY = (double)numUp->Value;
		Refresh();
	}

	void OnRX(System::Object ^sender, System::EventArgs ^e)
	{
		int index = functionsList->SelectedIndex;
		if (index == -1) return;

		if (numRX->Value < 0) numRX->Value = 0;
		((Function^)functionsList->Items[index])->op.scaleX = (double)(numRX->Value / 100);
		Refresh();

	}

	void OnRY(System::Object ^sender, System::EventArgs ^e)
	{
		int index = functionsList->SelectedIndex;
		if (index == -1) return;

		if (numRY->Value < 0) numRY->Value = 0;
		((Function^)functionsList->Items[index])->op.scaleY = (double)(numRY->Value / 100);
		Refresh();

	}
	void OnSelectedIndexChanged(System::Object ^sender, System::EventArgs ^e)
	{
		int index = functionsList->SelectedIndex;
		if (index == -1)
		{
			numRight->Enabled = false;
			numUp->Enabled = false;
			numRX->Enabled = false;
			numRY->Enabled = false;
		}
		else
		{
			numRight->Enabled = true;
			numUp->Enabled = true;
			numRX->Enabled = true;
			numRY->Enabled = true;
			numRight->Value = (Decimal)((Function^)functionsList->Items[index])->op.translationX;
			numUp->Value = (Decimal)((Function^)functionsList->Items[index])->op.translationY;
			numRX->Value = (Decimal)((Function^)functionsList->Items[index])->op.scaleX*100;
			numRY->Value = (Decimal)((Function^)functionsList->Items[index])->op.scaleY*100;
		}

	}
};

[System::STAThread]
int main()
{
	DateTime dt = DateTime(2018, 5, 7, 12, 0, 0, 0);

	Console::WriteLine((DateTime::Now - dt).TotalSeconds);

	myForm^ form1 = gcnew myForm();
	Application::Run(form1);
}