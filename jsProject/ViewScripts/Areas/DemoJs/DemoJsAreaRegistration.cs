using System.Web.Mvc;

namespace ViewScripts.Areas.DemoJs
{
    public class DemoJsAreaRegistration : AreaRegistration 
    {
        public override string AreaName 
        {
            get 
            {
                return "DemoJs";
            }
        }

        public override void RegisterArea(AreaRegistrationContext context) 
        {
            context.MapRoute(
                "DemoJs_default",
                "DemoJs/{controller}/{action}/{id}",
                new { action = "Index", id = UrlParameter.Optional }
            );
        }
    }
}