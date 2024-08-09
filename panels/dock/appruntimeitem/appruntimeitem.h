#include "applet.h"
#include "dsglobal.h"

DS_BEGIN_NAMESPACE
namespace dock {
class AppRuntimeItem : public DApplet
{
    Q_OBJECT
public:
    explicit AppRuntimeItem(QObject *parent = nullptr);

    Q_INVOKABLE void toggleruntimeitem();
};

}
DS_END_NAMESPACE
